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

#ifndef TDSL_DETAIL_COMMAND_CONTEXT_HPP
#define TDSL_DETAIL_COMMAND_CONTEXT_HPP

#include <tdslite/detail/tdsl_tds_context.hpp>
#include <tdslite/detail/tdsl_string_writer.hpp>
#include <tdslite/detail/tdsl_callback.hpp>
#include <tdslite/detail/tdsl_row.hpp>
#include <tdslite/detail/tdsl_token_handler_result.hpp>
#include <tdslite/detail/token/tds_done_token.hpp>
#include <tdslite/detail/token/tds_info_token.hpp>
#include <tdslite/detail/token/tds_colmetadata_token.hpp>
#include <tdslite/detail/tdsl_data_type.hpp>
#include <tdslite/detail/tdsl_sql_parameter.hpp>

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_string_view.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>

namespace tdsl { namespace detail {

    /**
     * Helper type to execute SQL commands
     *
     * @tparam NetImpl Network implementation type
     */
    template <typename NetImpl>
    struct command_context {
        using tds_context_type     = tds_context<NetImpl>;
        // Constant reference to tds_colmetadata_token
        using column_metadata_cref = const tdsl::tds_colmetadata_token &;
        // Constant reference to tdsl_row
        using row_cref             = const tdsl::tdsl_row &;
        using row_callback_fn_t    = void (*)(void *, column_metadata_cref, row_cref);

        struct command_options {
            struct {
                tdsl::uint8_t read_colnames : 1;
                tdsl::uint8_t reserved : 7;
            } flags = {};
        };

    private:
        using self_type                = command_context<NetImpl>;
        using string_writer_type       = string_parameter_writer<tds_context_type>;
        using nonquery_result_callback = callback<tds_done_token>;

        // --------------------------------------------------------------------------------

        tds_context_type & tds_ctx;
        command_options options;

    public:
        // --------------------------------------------------------------------------------

        /**
         * Default c-tor
         *
         * @param [in] ctx The TDS context associated with the command
         */
        command_context(tds_context_type & ctx, const command_options & opts = {}) noexcept :
            tds_ctx(ctx), options(opts) {

            tds_ctx.callbacks.sub_token_handler = {this, &token_handler};

            tds_ctx.callbacks.done              = {
                this, [](void * uptr, const tds_done_token & dt) noexcept -> void {
                    command_context & ctx    = *static_cast<command_context *>(uptr);
                    ctx.qstate.affected_rows = dt.done_row_count;
                    TDSL_DEBUG_PRINT("cc: done token -- affected rows(%d)\n", dt.done_row_count);
                }};
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
         * The result set returned by query @p command can be read by providing
         * a row callback function
         *
         * @return Number of rows affected
         */
        template <typename T, traits::enable_when::same_any_of<T, string_view, wstring_view,
                                                               struct progmem_string_view> = true>
        inline tdsl::uint32_t execute_query(
            T command, void * rcb_uptr = nullptr,
            row_callback_fn_t row_callback = +[](void *, const tds_colmetadata_token &,
                                                 const tdsl_row &) -> void {}) noexcept {
            // Reset query state object & reassign row callback
            qstate              = {};
            qstate.row_callback = {rcb_uptr, row_callback};
            // Write the SQL command
            string_writer_type::write(tds_ctx, command);
            // Send the command
            tds_ctx.send_tds_pdu(e_tds_message_type::sql_batch);
            // Receive the response
            tds_ctx.receive_tds_pdu();
            // The state will be updated upon receiving the response
            return qstate.affected_rows;
        }

        // --------------------------------------------------------------------------------

        enum class e_proc_id : tdsl::uint8_t
        {
            sp_cursor         = 1,
            sp_cursoropen     = 2,
            sp_cursorprepare  = 3,
            sp_cursorexecute  = 4,
            sp_cursorprepexec = 5,
            sp_cursorfetch    = 7,
            sp_cursoroption   = 8,
            sp_cursorclose    = 9,
            sp_executesql     = 10,
            sp_prepare        = 11,
            sp_execute        = 12,
            sp_prepexec       = 13,
            sp_prepexecrpc    = 14,
            sp_unprepare      = 15
        };

        enum class e_rpc_mode : tdsl::uint8_t
        {
            executesql = static_cast<tdsl::uint8_t>(e_proc_id::sp_executesql),
            prepexec   = static_cast<tdsl::uint8_t>(e_proc_id::sp_prepexec),
        };

        enum class e_rpc_error_code : tdsl::uint8_t
        {
            invalid_mode = 1,
        };

        using execute_rpc_result = tdsl::expected<tdsl::uint32_t, e_rpc_error_code>;

        template <typename T, traits::enable_when::same_any_of<T, string_view, wstring_view,
                                                               struct progmem_string_view> = true>
        inline execute_rpc_result execute_rpc(
            T command, tdsl::span<sql_parameter_binding> params = {},
            e_rpc_mode mode = {e_rpc_mode::executesql}, void * rcb_uptr = nullptr,
            row_callback_fn_t row_callback = +[](void *, const tds_colmetadata_token &,
                                                 const tdsl_row &) -> void {}) noexcept {
            // Validate mode
            switch (mode) {
                case e_rpc_mode::executesql:
                case e_rpc_mode::prepexec:
                    break;
                default:
                    return execute_rpc_result::unexpected(e_rpc_error_code::invalid_mode);
            }

            tds_ctx.write_le(static_cast<tdsl::uint16_t>(0xffff)); // procedure name length
            tds_ctx.write_le(static_cast<tdsl::uint16_t>(mode));   // stored procedure id
            tds_ctx.write_le(static_cast<tdsl::uint16_t>(0));      // option flags

            for (const auto & param : params) {
                tds_ctx.write_le(static_cast<tdsl::uint8_t>(0));          // name length
                tds_ctx.write_le(static_cast<tdsl::uint8_t>(0));          // status flags
                tds_ctx.write_le(static_cast<tdsl::uint8_t>(param.type)); // type
                const auto & dprops        = get_data_type_props(param.type);

                auto maybe_write_collation = [&]() {
                    if (dprops.flags.has_collation) {
                        // put collation data as well
                        tds_ctx.write_le(tdsl::uint32_t{0});
                        tds_ctx.write_le(tdsl::uint8_t{0});
                    }
                };
                switch (dprops.size_type) {
                    case e_tds_data_size_type::fixed:
                        // Do nothing.
                        break;
                    case e_tds_data_size_type::var_u8:
                        tds_ctx.write_le(static_cast<tdsl::uint8_t>(0xFF)); // max length - 1 byte
                        maybe_write_collation();
                        tds_ctx.write_le(static_cast<tdsl::uint8_t>(param.value.size_bytes()));
                        break;
                    case e_tds_data_size_type::var_u16:
                        tds_ctx.write_le(
                            static_cast<tdsl::uint16_t>(0xFFFF)); // max length - 2 bytes
                        maybe_write_collation();
                        tds_ctx.write_le(static_cast<tdsl::uint16_t>(param.value.size_bytes()));

                        break;
                    case e_tds_data_size_type::var_u32:
                        tds_ctx.write_le(0xFFFFFFFF); // max length - 2 bytes
                        maybe_write_collation();
                        tds_ctx.write_le(static_cast<tdsl::uint32_t>(param.value.size_bytes()));
                        break;
                    case e_tds_data_size_type::var_precision:
                        break;
                    case e_tds_data_size_type::unknown:
                        TDSL_TRAP;
                        TDSL_UNREACHABLE;
                        break;
                }

                tds_ctx.write(param.value);
            }

            // Parameter list

            // for each parameter in parameter list
            // name length - name
            // status flags
            // type info
            //  type
            //  maxlen??
            // value
            //  length
            //  data

            /**
             *  TYPE_INFO=FIXEDLENTYPE
                /
                (VARLENTYPE TYPE_VARLEN [COLLATION])
                /
                (VARLENTYPE TYPE_VARLEN [PRECISION SCALE])
                /
                (VARLENTYPE SCALE) ; (introduced in TDS 7.3)
                /
                VARLENTYPE
                ; (introduced in TDS 7.3)
                /
                (PARTLENTYPE
                [USHORTMAXLEN]
             *
             */

            // sp_execute format
            // SELECT * FROM TEST WHERE A = @name, @name int, @name = 5

            // SELECT * FROM TEST WHERE A = @name
            // paramlist
            //  name|int|AABBCCDD

            // parameter* parameter_count;

            // select * from q where x = @P1;

            // Reset query state object & reassign row callback
            qstate              = {};
            qstate.row_callback = {rcb_uptr, row_callback};
            // Write the SQL command
            string_writer_type::write(tds_ctx, command);
            // Send the command
            tds_ctx.send_tds_pdu(e_tds_message_type::rpc);
            // Receive the response
            tds_ctx.receive_tds_pdu();
            // The state will be updated upon receiving the response
            return tdsl::uint32_t{qstate.affected_rows};
        }

        // --------------------------------------------------------------------------------

        /**
         * Token handler for command_context.
         *
         * Handles COLMETADATA & ROW token types.
         *
         * @param [in] uptr User-ptr (command context instance)
         * @param [in] token_type Type of the token to handle
         * @param [in] rr Binary reader
         *
         * @returns token-specific handler result for COLMETADATA & ROW tokens
         * @returns default token handler result, which is `unhandled`
         */
        static TDSL_NODISCARD token_handler_result
        token_handler(void * uptr, e_tds_message_token_type token_type,
                      tdsl::binary_reader<tdsl::endian::little> & rr) noexcept {
            TDSL_ASSERT(uptr);
            self_type & self = *static_cast<self_type *>(uptr);
            switch (token_type) {
                case e_tds_message_token_type::colmetadata:
                    return self.handle_colmetadata_token(rr);
                case e_tds_message_token_type::row:
                    return self.handle_row_token(rr);
                default:
                    return {};
            }
            TDSL_UNREACHABLE;
        }

    private:
        /**
         * The query state object.
         */
        struct query_state {
            /**
             * Column metadata for current query (if applicable)
             */
            tds_colmetadata_token colmd                    = {};
            /**
             * Number of affected rows from current query
             */
            tdsl::uint32_t affected_rows                   = {0};
            /**
             * If the query returns a result set, this is the
             * function to be called for every row read from
             * the result set.
             */
            callback<void, row_callback_fn_t> row_callback = {};

        } qstate = {};

        // --------------------------------------------------------------------------------

        /**
         * Handler for COLMETADATA token type
         *
         * The function parses the given data in @p rr as COLMETADATA token and calls the info
         * callback function, if a callback function is assigned.
         *
         * @param [in] rr Reader to read from
         *
         * @return tdsl::uint32_t Amount of needed bytes to read a complete COLMETADATA token, if
         * any. The return value would be non-zero only if the reader has partial token data.
         */
        TDSL_NODISCARD token_handler_result
        handle_colmetadata_token(tdsl::binary_reader<tdsl::endian::little> & rr) noexcept {
            token_handler_result result                   = {};
            constexpr static auto k_min_colmetadata_bytes = 8;
            if (not rr.has_bytes(k_min_colmetadata_bytes)) {
                result.status       = token_handler_status::not_enough_bytes;
                result.needed_bytes = k_min_colmetadata_bytes - rr.remaining_bytes();
                TDSL_DEBUG_PRINTLN(
                    "received COLMETADATA token, not enough bytes need (at least) %d, have %zu",
                    k_min_colmetadata_bytes, rr.remaining_bytes());
                return result;
            }

            // Read colum count, try to allocate memory for N columns
            const auto column_count = rr.read<tdsl::uint16_t>();
            if (not qstate.colmd.allocate_colinfo_array(column_count)) {
                result.status = token_handler_status::not_enough_memory;
                TDSL_DEBUG_PRINTLN("failed to allocate memory for column info for %d column(s)",
                                   column_count);
                return result;
            }

            // If the user opted in for reading the column names,
            // allocate memory for the column name array.
            // Note that this just allocates memory for the pointers,
            // not the actual column names.
            if (options.flags.read_colnames) {
                // Allocate column name array
                if (not qstate.colmd.allocate_column_name_array(column_count)) {
                    result.status = token_handler_status::not_enough_memory;
                    TDSL_DEBUG_PRINTLN(
                        "failed to allocate memory for column name array for %d column(s)",
                        column_count);
                    return result;
                }
            }

            // Absolute minimum COLMETADATA bytes, regardless of data type
            constexpr static auto k_colinfo_min_bytes = 6; // user_type + flags + type + colname len

            auto colindex                             = 0;

            // Main column metadata read loop
            while (colindex < column_count && rr.has_bytes(k_colinfo_min_bytes)) {
                tds_column_info & current_column = qstate.colmd.columns [colindex];
                current_column.user_type         = rr.read<tdsl::uint16_t>();
                current_column.flags             = rr.read<tdsl::uint16_t>();
                current_column.type    = static_cast<e_tds_data_type>(rr.read<tdsl::uint8_t>());

                const auto dtype_props = get_data_type_props(current_column.type);

                if (not rr.has_bytes(/*amount_of_bytes=*/dtype_props.min_colmetadata_size())) {
                    result.status       = token_handler_status::not_enough_bytes;
                    result.needed_bytes = dtype_props.min_colmetadata_size() - rr.remaining_bytes();
                    return result;
                }

                // column info layout depends on column's size type and
                // other factors like collation.
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

                        TDSL_DEBUG_PRINTLN(
                            "unable to determine data type size for type %d, aborting read",
                            static_cast<tdsl::uint8_t>(current_column.type));
                        result.status       = token_handler_status::unknown_column_size_type;
                        result.needed_bytes = 0;
                        return result;
                }

                // If data type has collation info:
                if (dtype_props.flags.has_collation) {
                    constexpr int k_collation_info_size = 5;
                    if (not rr.has_bytes(k_collation_info_size)) {
                        result.status       = token_handler_status::not_enough_bytes;
                        result.needed_bytes = k_collation_info_size - rr.remaining_bytes();
                        TDSL_DEBUG_PRINTLN(
                            "not enough bytes to read collation information, need %d, have %ld",
                            k_collation_info_size, rr.remaining_bytes());
                        return result;
                    }
                    // FIXME: Read this info into current_column
                    rr.advance(k_collation_info_size);

                    TDSL_DEBUG_PRINTLN("command_context::handle_colmetadata_token(...) -> colidx "
                                       "%d collinfo: \n",
                                       colindex);
                }

                // If data type has table name info:
                if (dtype_props.flags.has_table_name) {
                    TDSL_DEBUG_PRINTLN("command_context::handle_colmetadata_token(...) -> colidx "
                                       "%d has table name\n",
                                       colindex);
                    do {
                        constexpr int k_table_name_size_len = 2;
                        auto tname_needed_bytes             = k_table_name_size_len;
                        if (rr.has_bytes(tname_needed_bytes)) {
                            const auto table_name_length_in_chars = rr.read<tdsl::uint16_t>();
                            tname_needed_bytes = (table_name_length_in_chars * 2);
                            if (rr.has_bytes(tname_needed_bytes)) {
                                // FIXME: Read this info into current_column?
                                rr.advance(tname_needed_bytes);
                                break;
                            }
                        }
                        result.status       = token_handler_status::not_enough_bytes;
                        result.needed_bytes = k_table_name_size_len - rr.remaining_bytes();
                        TDSL_DEBUG_PRINTLN("not enough bytes to read table name, need %d, have %ld",
                                           tname_needed_bytes, rr.remaining_bytes());
                        return result;
                    } while (0);
                }

                // Read column name
                current_column.colname_length_in_chars = rr.read<tdsl::uint8_t>();
                const auto colname_len_in_bytes = (current_column.colname_length_in_chars * 2);
                if (not rr.has_bytes((colname_len_in_bytes))) {
                    result.status       = token_handler_status::not_enough_bytes;
                    result.needed_bytes = colname_len_in_bytes - rr.remaining_bytes();
                    TDSL_DEBUG_PRINTLN("not enough bytes to read column name, need %d, have %ld",
                                       colname_len_in_bytes, rr.remaining_bytes());
                    return result;
                }

                if (not options.flags.read_colnames) {
                    TDSL_EXPECT(rr.advance(colname_len_in_bytes));
                }
                else {
                    const auto span = rr.read(colname_len_in_bytes);
                    if (span && not qstate.colmd.set_column_name(colindex, span)) {
                        result.status = token_handler_status::not_enough_memory;
                        TDSL_DEBUG_PRINTLN("failed to allocate memory for column index %d 's name",
                                           colindex);
                        return result;
                    }
                }
                ++colindex;
            }

            TDSL_DEBUG_PRINTLN("received COLMETADATA token -> column count [%zu]",
                               qstate.colmd.columns.size());
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
         * @return token_handler_result
         */
        TDSL_NODISCARD token_handler_result
        handle_row_token(tdsl::binary_reader<tdsl::endian::little> & rr) noexcept {
            using data_size_type        = tdsl::detail::e_tds_data_size_type;

            token_handler_result result = {};
            // Invoke handler,return
            if (not qstate.colmd) {
                // encountered row info without colmetadata?
                TDSL_DEBUG_PRINTLN(
                    "Encountered ROW token withour prior COLMETADATA token, discarding packet");
                result.status = token_handler_status::missing_prior_colmetadata;
                return result;
            }

            auto row_data{tdsl_row::make(qstate.colmd.columns.size())};

            if (not row_data) {
                TDSL_DEBUG_PRINTLN("row data creation failed (%d)",
                                   static_cast<int>(row_data.error()));
                // report error
                result.status = token_handler_status::not_enough_memory;
                return result;
            }

            // Each row should contain N fields.
            for (tdsl::uint32_t cidx = 0; cidx < qstate.colmd.columns.size(); cidx++) {
                TDSL_ASSERT(cidx < row_data->size());
                const auto & column             = qstate.colmd.columns [cidx];
                const auto & dprop              = get_data_type_props(column.type);
                auto & field                    = (*row_data) [cidx];
                bool field_length_equal_to_null = {false};

                // Allow me to present yet another nonsense from TDS:
                if (dprop.flags.has_textptr) {
                    // non-null text, ntext or img field.
                    // FIXME: Is this any useful?
                    do {
                        auto textptr_need_bytes = 1;
                        if (rr.has_bytes(textptr_need_bytes)) {
                            textptr_need_bytes = rr.read<tdsl::uint8_t>();
                            if (textptr_need_bytes == 0xFF) {
                                // Revert read & break
                                rr.advance(/*amount_of_bytes=*/-1);
                                break;
                            }
                            if (rr.has_bytes(textptr_need_bytes)) {
                                rr.advance(textptr_need_bytes);
                                textptr_need_bytes = 8;
                                if (rr.has_bytes(textptr_need_bytes)) {
                                    rr.advance(textptr_need_bytes);
                                    TDSL_DEBUG_PRINTLN("textptr skip exit");
                                    break;
                                }
                            }
                        }

                        TDSL_DEBUG_PRINTLN("handle_row_token() --> not enough bytes for reading "
                                           "field textptr, %lu more bytes needed",
                                           textptr_need_bytes - rr.remaining_bytes());
                        result.status       = token_handler_status::not_enough_bytes;
                        result.needed_bytes = textptr_need_bytes - rr.remaining_bytes();
                        return result;
                    } while (0);
                }

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
                        field_length               = rr.read<tdsl::uint8_t>();
                        field_length_equal_to_null = dprop.flags.zero_represents_null &&
                                                     (field_length == tdsl::uint8_t{0x00});
                        break;
                    case data_size_type::var_u16:
                        if (not rr.has_bytes(sizeof(tdsl::uint16_t))) {
                            result.status       = token_handler_status::not_enough_bytes;
                            result.needed_bytes = sizeof(tdsl::uint16_t);
                            return result;
                        }
                        field_length               = rr.read<tdsl::uint16_t>();
                        field_length_equal_to_null = dprop.flags.maxlen_represents_null &&
                                                     (field_length == tdsl::uint16_t{0xFFFF});
                        break;
                    case data_size_type::var_u32:
                        if (not rr.has_bytes(sizeof(tdsl::uint32_t))) {
                            result.status       = token_handler_status::not_enough_bytes;
                            result.needed_bytes = sizeof(tdsl::uint32_t);
                            return result;
                        }
                        field_length               = rr.read<tdsl::uint32_t>();
                        field_length_equal_to_null = dprop.flags.maxlen_represents_null &&
                                                     (field_length == tdsl::uint32_t{0xFFFFFFFF});
                        break;
                    case data_size_type::unknown:
                        TDSL_ASSERT_MSG(0, "unknown size_type");
                        TDSL_UNREACHABLE;
                        break;
                }

                // TEXT, NTEXT, IMAGE

                if (dprop.is_variable_size() &&
                    not is_valid_variable_length_for_type(column.type, field_length)) {
                    TDSL_DEBUG_PRINTLN(
                        "handle_row_token() --> invalid varlength for column type %d -> %d",
                        static_cast<int>(column.type), field_length);
                    result.status = token_handler_status::invalid_field_length;
                    return result;
                }

                if (field_length_equal_to_null) {
                    field_length = {0};
                    field.set_null();
                }

                if (not rr.has_bytes(field_length)) {
                    TDSL_DEBUG_PRINTLN("handle_row_token() --> not enough bytes for reading field, "
                                       "%lu more bytes needed",
                                       field_length - rr.remaining_bytes());
                    result.status       = token_handler_status::not_enough_bytes;
                    result.needed_bytes = field_length - rr.remaining_bytes();
                    return result;
                }

                if (field_length) {
                    field = rr.read(field_length);
                }
                // (mgilor): '%.*s' does not function here; printf stops writing
                // characters when it reaches a \0 (NUL), regardless of the actual 
                // length of the provided string.
                TDSL_DEBUG_PRINT("row field %u -> [", cidx);
                TDSL_DEBUG_HEXPRINT(field.data(), field.size_bytes());
                TDSL_DEBUG_PRINT("]\n");
            }

            // Invoke row callback
            qstate.row_callback(qstate.colmd, row_data.get());

            result.status       = token_handler_status::success;
            result.needed_bytes = 0;
            return result;
        }
    };
}} // namespace tdsl::detail

#endif