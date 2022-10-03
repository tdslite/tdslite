/**
 * _________________________________________________
 *
 * @file   tdsl_command_context.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   23.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/detail/tdsl_tds_context.hpp>
#include <tdslite/detail/tdsl_string_writer.hpp>
#include <tdslite/detail/tdsl_callback.hpp>
#include <tdslite/detail/tdsl_row.hpp>
#include <tdslite/detail/tdsl_token_handler_result.hpp>
#include <tdslite/detail/token/tdsl_done_token.hpp>
#include <tdslite/detail/token/tdsl_info_token.hpp>
#include <tdslite/detail/token/tdsl_colmetadata_token.hpp>

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_string_view.hpp>

#include <tdslite/util/tdsl_type_traits.hpp>

namespace tdsl { namespace detail {

    template <typename NetImpl>
    struct command_context {
        using tds_context_type         = tds_context<NetImpl>;
        using self_type                = command_context<NetImpl>;
        using string_writer_type       = string_parameter_writer<tds_context_type>;
        using nonquery_result_callback = callback<tds_done_token>;

        using row_callback_fn_t        = void (*)(void *, const tds_colmetadata_token &, const tdsl_row &);

        // --------------------------------------------------------------------------------

        /**
         * Default c-tor
         *
         * @param [in] ctx The TDS context associated with the command
         */
        command_context(tds_context_type & ctx) : tds_ctx(ctx) {

            tds_ctx.do_register_sub_token_handler(this, &token_handler);
            tds_ctx.do_register_info_token_callback(
                this, +[](void *, const tds_info_token &) -> tdsl::uint32_t {
                    return 0;
                });
            tds_ctx.do_register_done_token_callback(this, [](void * uptr, const tds_done_token & dt) -> tdsl::uint32_t {
                command_context & ctx          = *static_cast<command_context *>(uptr);
                ctx.qstate.affected_rows       = dt.done_row_count;
                ctx.qstate.flags.received_done = true;
                TDSL_DEBUG_PRINT("received done token %d\n", dt.done_row_count);
                return 0;
            });
        }

        // --------------------------------------------------------------------------------

        /**
         * Execute a query
         *
         * @tparam T Auto-deduced string type (char_span or u16char_span)
         *
         * @param [in] command SQL command to execute
         * @param [in] rcb_uptr Row callback user pointer (optional)
         * @param [in] row_callback Row callback function (optional)
         *
         * The result set returned by query @p command can be read by providing a
         * row callback function
         *
         * @return Rows affected
         */
        template <typename T, traits::enable_if_same_any<T, string_view, wstring_view> = true>
        inline tdsl::uint32_t execute_query(
            T command, void * rcb_uptr = nullptr,
            row_callback_fn_t row_callback = +[](void *, const tds_colmetadata_token &, const tdsl_row &) {}) noexcept {

            qstate.reset();

            qstate.row_callback.set(rcb_uptr, row_callback);

            // Write the TDS header for the command
            tds_ctx.write_tds_header(e_tds_message_type::sql_batch);
            // Write the SQL command
            string_writer_type::write(tds_ctx, command);
            // Put the data length (size of the SQL command)
            tds_ctx.put_tds_header_length(string_writer_type::calculate_write_size(command));
            // Send the command
            tds_ctx.send();

            // Receive the response
            // FIXME: receive until seeing DONE token?

            // invoke RECV unitl seeing the DONE token.
            do {
                tds_ctx.recv(8);
            } while (not qstate.flags.received_done);

            return qstate.affected_rows;
        }

        // --------------------------------------------------------------------------------

        static token_handler_result token_handler(void * uptr, e_tds_message_token_type token_type,
                                                  tdsl::binary_reader<tdsl::endian::little> & rr) {
            TDSL_ASSERT(uptr);
            self_type & self = *static_cast<self_type *>(uptr);

            switch (token_type) {
                case e_tds_message_token_type::colmetadata:
                    return self.handle_colmetadata_token(rr);
                case e_tds_message_token_type::row:
                    return self.handle_row_token(rr);
            }
            token_handler_result default_r{};
            TDSL_ASSERT(default_r.status == token_handler_status::unhandled);
            return default_r;
        }

    private:
        struct query_state {
            tds_colmetadata_token colmd;
            tdsl::uint32_t affected_rows;
            callback<void, row_callback_fn_t> row_callback{};

            struct {
                tdsl::uint8_t received_done : 1;
                tdsl::uint8_t reserved : 7;
            } flags;

            inline void reset() {
                colmd.reset();
                affected_rows = 0;
                row_callback.set(/*user_ptr=*/nullptr, /*callback=*/nullptr);
            }
        } qstate;

        // --------------------------------------------------------------------------------

        /**
         * Handler for COLMETADATA token type
         *
         * The function parses the given data in @p rr as COLMETADATA token and calls the info
         * callback function, if a callback function is assigned.
         *
         * @param [in] rr Reader to read from
         * @param [out] out Variable to put parsed COLMETADATA token data
         *
         * @return tdsl::uint32_t Amount of needed bytes to read a complete COLMETADATA token, if any.
         *                        The return value would be non-zero only if the reader has partial
         *                        token data.
         */
        token_handler_result handle_colmetadata_token(tdsl::binary_reader<tdsl::endian::little> & rr) {
            token_handler_result result{};

            constexpr static auto k_min_colmetadata_bytes = 8;
            if (not rr.has_bytes(k_min_colmetadata_bytes)) {
                result.status       = token_handler_status::not_enough_bytes;
                result.needed_bytes = k_min_colmetadata_bytes - rr.remaining_bytes();
                TDSL_DEBUG_PRINTLN("received COLMETADATA token, not enough bytes need (at least) %d, have %ld", k_min_colmetadata_bytes,
                                   rr.remaining_bytes());
                return result;
            }

            auto column_count = rr.read<tdsl::uint16_t>();
            if (not qstate.colmd.allocate_colinfo_array(column_count)) {
                result.status = token_handler_status::not_enough_memory;
                TDSL_DEBUG_PRINTLN("failed to allocate memory for column info for %d column(s)", column_count);
                return result;
            }

            if (flags.read_colnames) {
                // Allocate column name array
                if (not qstate.colmd.allocate_column_name_array(column_count)) {
                    result.status = token_handler_status::not_enough_memory;
                    TDSL_DEBUG_PRINTLN("failed to allocate memory for column name array for %d column(s)", column_count);
                    return result;
                }
            }

            // Absolute minimum COLMETADATA bytes, regardless of data type
            constexpr int k_colinfo_min_bytes = 6; // user_type + flags + type + colname len
            auto colindex                     = 0;
            while (colindex < column_count && rr.has_bytes(k_colinfo_min_bytes)) {
                tds_column_info & current_column = qstate.colmd.columns [colindex];

                // keep column names separate? makes sense. allocate only if user decided to see column names.
                current_column.user_type         = rr.read<tdsl::uint16_t>();
                current_column.flags             = rr.read<tdsl::uint16_t>();
                current_column.type              = static_cast<e_tds_data_type>(rr.read<tdsl::uint8_t>());

                const auto dtype_props           = get_data_type_props(current_column.type);

                if (not rr.has_bytes(/*amount_of_bytes=*/dtype_props.min_colmetadata_size())) {
                    result.status       = token_handler_status::not_enough_bytes;
                    result.needed_bytes = dtype_props.min_colmetadata_size() - rr.remaining_bytes();
                    return result;
                }

                // COLLATION occurs only if the type is BIGCHARTYPE, BIGVARCHARTYPE, TEXTTYPE, NTEXTTYPE,
                // NCHARTYPE, or NVARCHARTYPE.

                switch (dtype_props.size_type) {
                    case e_tds_data_size_type::fixed:
                        // No extra info to read for these
                        // types. Their length is fixed.
                        break;
                    case e_tds_data_size_type::var_u8:
                        // Variable length data types with 8-bit length bit width
                        current_column.typeprops.u8l.length = rr.read<tdsl::uint8_t>();
                        break;
                    case e_tds_data_size_type::var_u16:
                        // Variable length data types with 16-bit length bit width
                        current_column.typeprops.u16l.length = rr.read<tdsl::uint16_t>();
                        break;
                    case e_tds_data_size_type::var_u32:
                        // Variable length data types with 32-bit length bit width
                        current_column.typeprops.u32l.length = rr.read<tdsl::uint32_t>();
                        break;
                    case e_tds_data_size_type::var_precision:
                        // Types like DECIMALNTYPE & NUMERICNTYPE have precision
                        // and scale values. Precision determines the field's length
                        // whereas scale is the multiplier.
                        current_column.typeprops.ps.length    = rr.read<tdsl::uint8_t>();
                        current_column.typeprops.ps.precision = rr.read<tdsl::uint8_t>();
                        current_column.typeprops.ps.scale     = rr.read<tdsl::uint8_t>();
                        break;
                    case e_tds_data_size_type::unknown:

                        TDSL_DEBUG_PRINTLN("unable to determine data type size for type %d, aborting read",
                                           static_cast<tdsl::uint8_t>(current_column.type));
                        result.status       = token_handler_status::unknown_column_size_type;
                        result.needed_bytes = 0;
                        return result;
                }

                // If data type has collation info, read it
                if (dtype_props.flags.has_collation) {
                    constexpr int k_collation_info_size = 5;
                    if (not rr.has_bytes(k_collation_info_size)) {
                        result.status       = token_handler_status::not_enough_bytes;
                        result.needed_bytes = k_collation_info_size - rr.remaining_bytes();
                        TDSL_DEBUG_PRINTLN("not enough bytes to read collation information, need %d, have %ld", k_collation_info_size,
                                           rr.remaining_bytes());
                        return result;
                    }
                    // FIXME: Read this info into current_column
                    rr.advance(k_collation_info_size);
                }

                current_column.colname_length_in_chars = rr.read<tdsl::uint8_t>();
                const auto colname_len_in_bytes        = (current_column.colname_length_in_chars * 2);
                if (not rr.has_bytes((colname_len_in_bytes * 2))) {
                    result.status       = token_handler_status::not_enough_bytes;
                    result.needed_bytes = colname_len_in_bytes - rr.remaining_bytes();
                    TDSL_DEBUG_PRINTLN("not enough bytes to read column name, need %d, have %ld", colname_len_in_bytes,
                                       rr.remaining_bytes());
                    return result;
                }

                if (not flags.read_colnames) {
                    TDSL_EXPECT(rr.advance(colname_len_in_bytes));
                }
                else {
                    const auto span = rr.read(colname_len_in_bytes);
                    if (not qstate.colmd.set_column_name(colindex, span)) {
                        result.status = token_handler_status::not_enough_memory;
                        TDSL_DEBUG_PRINTLN("failed to allocate memory for column index %d 's name", colindex);
                        return result;
                    }
                }
                ++colindex;
            }

            TDSL_DEBUG_PRINTLN("received COLMETADATA token -> column count [%d]", qstate.colmd.column_count);
            result.status       = token_handler_status::success;
            result.needed_bytes = 0;
            return result;
        }

        // --------------------------------------------------------------------------------

        /**
         * Handler for ROW token type
         *
         * The function parses the given data in @p rr as DONE token and calls the info
         * callback function, if a callback function is assigned.
         *
         * @param [in] rr Reader to read from
         *
         * @return tdsl::uint32_t Amount of needed bytes to read a complete DONE token, if any.
         *                        The return value would be non-zero only if the reader has partial
         *                        token data.
         */
        token_handler_result handle_row_token(tdsl::binary_reader<tdsl::endian::little> & rr) {

            token_handler_result result{};
            // Invoke handler,return
            if (not qstate.colmd.is_valid()) {
                // encountered row info without colmetadata?
                TDSL_DEBUG_PRINTLN("Encountered ROW token withour prior COLMETADATA token, discarding packet");
                result.status = token_handler_status::missing_prior_colmetadata;
                return result;
            }

            // using data_type                        = tdsl::detail::e_tds_data_type;
            using data_size_type = tdsl::detail::e_tds_data_size_type;

            // constexpr static auto k_min_done_bytes = 8;

            // if (not rr.has_bytes(k_min_done_bytes)) {
            //     return k_min_done_bytes - rr.remaining_bytes();
            // }

            auto row_data{tdsl_row::make(qstate.colmd.column_count)};

            if (not row_data) {
                TDSL_DEBUG_PRINTLN("row data creation failed (%d)", static_cast<int>(row_data.error()));
                // report error
                result.status = token_handler_status::not_enough_memory;
                return result;
            }

            // Each row should contain N fields.
            for (tdsl::uint32_t cidx = 0; cidx < qstate.colmd.column_count; cidx++) {
                // TDSL_ASSERT_MSG(colmetadata.columns [cidx] != nullptr, "column info is null!");
                auto & field                = row_data->fields.data() [cidx];
                const auto & column         = qstate.colmd.columns [cidx];
                const auto & dprop          = get_data_type_props(column.type);

                tdsl::uint32_t field_length = 0;
                switch (dprop.size_type) {
                    case data_size_type::fixed:
                        field_length = dprop.length.fixed;
                        break;
                    case data_size_type::var_u8:
                    case data_size_type::var_precision:
                        if (not rr.has_bytes(sizeof(tdsl::uint8_t))) {
                            result.status       = token_handler_status::not_enough_bytes;
                            result.needed_bytes = 1;
                            return result;
                        }
                        field_length = rr.read<tdsl::uint8_t>();
                        break;
                    case data_size_type::var_u16:
                        if (not rr.has_bytes(sizeof(tdsl::uint16_t))) {
                            result.status       = token_handler_status::not_enough_bytes;
                            result.needed_bytes = sizeof(tdsl::uint16_t);
                            return result;
                        }
                        field_length = rr.read<tdsl::uint16_t>();
                        break;
                    case data_size_type::var_u32:
                        if (not rr.has_bytes(sizeof(tdsl::uint32_t))) {
                            result.status       = token_handler_status::not_enough_bytes;
                            result.needed_bytes = sizeof(tdsl::uint32_t);
                            return result;
                        }
                        field_length = rr.read<tdsl::uint32_t>();
                        break;
                    case data_size_type::unknown:
                        TDSL_ASSERT_MSG(0, "unknown size_type");
                        TDSL_UNREACHABLE;
                        break;
                }

                if (not rr.has_bytes(field_length)) {
                    TDSL_DEBUG_PRINTLN("not enough bytes for reading field, %lu more bytes needed", field_length - rr.remaining_bytes());
                    result.status       = token_handler_status::not_enough_bytes;
                    result.needed_bytes = field_length - rr.remaining_bytes();
                    return result;
                }

                if (field_length) {
                    field.data = rr.read(field_length);
                }

                // (mgilor) `%.*s` does not work here, printf stops printing
                // characters immediately when it encounters a \0, regardless
                // of the provided string's length.

                TDSL_DEBUG_PRINT("row field %u -> [", cidx);
                TDSL_DEBUG_HEXPRINT(field.data.data(), field_length);
                TDSL_DEBUG_PRINT("]\n");
            }

            // Invoke row callback
            qstate.row_callback.maybe_invoke(qstate.colmd, row_data.get());

            result.status       = token_handler_status::success;
            result.needed_bytes = 0;
            return result;
        }

        tds_context_type & tds_ctx;

        struct {
            tdsl::uint8_t read_colnames : 1;
            tdsl::uint8_t reserved : 7;
        } flags;
    };
}} // namespace tdsl::detail