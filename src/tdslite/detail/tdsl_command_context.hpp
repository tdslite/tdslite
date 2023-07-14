/**
 * ____________________________________________________
 * tdslite's SQL command execution context
 *
 * @file   tdsl_command_context.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   23.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
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
#include <tdslite/detail/tdsl_tds_procedure_id.hpp>

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_string_view.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_utos.hpp>

namespace tdsl { namespace detail {

    /**
     * Helper type to execute SQL commands
     *
     * The context is designed to be constructed
     * on demand.
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
        using execute_rpc_result   = tdsl::expected<tdsl::uint32_t, e_rpc_error_code>;

        struct command_options {
            struct {
                tdsl::uint8_t read_colnames : 1;
                tdsl::uint8_t reserved : 7;
            } flags = {};
        };

        struct query_result {

            /**
             * Number of affected rows from current query
             */
            tdsl::uint32_t affected_rows       = {0};

            /**
             *
             *
             */
            tds_done_token::status_type status = {};

            inline explicit operator bool() const noexcept {
                return !(status.error() || status.srverror());
            }
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
        inline command_context(tds_context_type & ctx, const command_options & opts = {}) noexcept :
            tds_ctx(ctx), options(opts) {

            tds_ctx.callbacks.sub_token_handler = {&token_handler, this};

            tds_ctx.callbacks.done              = {
                [](void * uptr, const tds_done_token & dt) noexcept -> void {
                    command_context & ctx           = *static_cast<command_context *>(uptr);
                    ctx.qstate.result.status        = dt.status;
                    ctx.qstate.result.affected_rows = dt.done_row_count;
                    TDSL_DEBUG_PRINT("cc: done token -- affected rows(%d)\n", dt.done_row_count);
                },
                this};
        }

        // --------------------------------------------------------------------------------

        /**
         * Execute a query
         *
         * @tparam T Auto-deduced string type (char_span or u16char_span)
         *
         * @param [in] command SQL command to execute
         * @param [in] row_callback Row callback function (optional)
         * @param [in] rcb_uptr Row callback user pointer (optional)
         *
         * The result set returned by query @p command can be read by providing
         * a row callback function
         *
         * @return Number of rows affected
         */
        template <typename T, traits::enable_when::same_any_of<T, string_view, wstring_view,
                                                               struct progmem_string_view> = true>
        inline query_result execute_query(
            T command,
            row_callback_fn_t row_callback = +[](void *, const tds_colmetadata_token &,
                                                 const tdsl_row &) -> void {},
            void * rcb_uptr                = nullptr) noexcept {
            // Reset query state object & reassign row callback
            qstate              = {};
            qstate.row_callback = {row_callback, rcb_uptr};
            // Write the SQL command
            string_writer_type::write(tds_ctx, command);
            // Send the command
            tds_ctx.send_tds_pdu(e_tds_message_type::sql_batch);
            // Receive the response
            tds_ctx.receive_tds_pdu();

            // The state will be updated upon receiving the response
            return qstate.result;
        }

        // --------------------------------------------------------------------------------

        /**
         * Perform a remote procedure call (e.g. execute a stored procedure or
         * a parameterized query)
         *
         * @tparam T String view type
         *
         * @param [in] command Command to execute
         * @param [in] params Parameters of the command, if any
         * @param [in] mode RPC execution mode
         * @param [in] row_callback Row callback function (optional)
         * @param [in] rcb_uptr Row callback user pointer (optional)
         *
         * The result set returned by query @p command can be read by providing
         * a row callback function
         *
         * @returns execute_rpc_result::unexpected(e_rpc_error_code::invalid_mode) if @p mode
         *          value is invalid
         * @returns rows_affected if successful
         */
        template <typename T, traits::enable_when::same_any_of<T, string_view, wstring_view,
                                                               struct progmem_string_view> = true>
        inline execute_rpc_result execute_rpc(
            T command, tdsl::span<sql_parameter_binding> params = {},
            e_rpc_mode mode                = e_rpc_mode::executesql,
            row_callback_fn_t row_callback = +[](void *, const tds_colmetadata_token &,
                                                 const tdsl_row &) -> void {},
            void * rcb_uptr                = nullptr) noexcept {
            // Validate mode
            switch (mode) {
                // Allowed & supported modes
                case e_rpc_mode::executesql:
                    break;
                case e_rpc_mode::prepexec:
                    TDSL_NOT_YET_IMPLEMENTED;
                    break;
                default:
                    return execute_rpc_result::unexpected(e_rpc_error_code::invalid_mode);
            }

            // 0xffff means we're going to use special procedure id
            // instead of a procedure name.
            tds_ctx.write_le(tdsl::uint16_t{0xffff});            // procedure name length
            tds_ctx.write_le(static_cast<tdsl::uint16_t>(mode)); // stored procedure id
            tds_ctx.write_le(tdsl::uint16_t{0});                 // option flags

            // sp_executesql expects @statement, @params and param values in order

            // Step 1:
            // Write the SQL command
            {
                tds_ctx.write(tdsl::uint8_t{0}); // name len
                tds_ctx.write(tdsl::uint8_t{0}); // status flags
                tds_ctx.write(static_cast<tdsl::uint8_t>(e_tds_data_type::NVARCHARTYPE)); // type
                tds_ctx.write(tdsl::uint16_t{8000});                                      // maxlen
                tds_ctx.write(tdsl::uint8_t{0});  // collation
                tds_ctx.write(tdsl::uint32_t{0}); // collation

                // write_type_info

                tds_ctx.write(
                    static_cast<tdsl::uint16_t>(string_writer_type::calculate_write_size(command)));
                string_writer_type::write(tds_ctx, command);
            }

            // Step 2:
            // Write parameter decls
            {
                tds_ctx.write(tdsl::uint8_t{0}); // name len
                tds_ctx.write(tdsl::uint8_t{0}); // status flags
                tds_ctx.write(static_cast<tdsl::uint8_t>(e_tds_data_type::NVARCHARTYPE)); // type
                tds_ctx.write(tdsl::uint16_t{8000});                                      // maxlen
                tds_ctx.write(tdsl::uint8_t{0});  // collation
                tds_ctx.write(tdsl::uint32_t{0}); // collation

                // put a placeholder
                auto param_decl_sz_ph  = tds_ctx.put_placeholder(tdsl::uint16_t{0});

                auto cw                = string_writer_type::make_counted_writer(tds_ctx);
                tdsl::size_t param_idx = {0};
                for (const auto & param : params) {
                    char utos_buf [10] = {0};
                    // written output should look like this
                    // @p1 int, @p2 varchar(30), @p3 int
                    tdsl::string_view param_decl{/*str=*/"@p"};
                    tdsl::string_view param_idx_str{tdsl::utos(param_idx++, utos_buf)};
                    cw.write(param_decl);
                    cw.write(param_idx_str);
                    cw.write(" ");
                    write_param_type_str(param, cw);
                    write_param_len_str(param, cw);

                    if (param_idx == params.size()) {
                        break;
                    }
                    cw.write(",");
                }

                // Write parameter declaration string length (in bytes)
                param_decl_sz_ph.write_le(static_cast<tdsl::uint16_t>(cw.get()));
            }

            // Write the parameter list
            for (const auto & param : params) {
                // We're not going to use parameter names in order
                // to save space. Instead, we'll put the values in
                // their declaration order.
                tds_ctx.write_le(tdsl::uint8_t{0}); // name length

                // I haven't able to find any use case for
                // this (yet) so, not used ATM.
                tds_ctx.write_le(tdsl::uint8_t{0}); // status flags

                auto type           = param.type;
                auto type_size      = param.type_size;

                // Data type properties
                const auto & dprops = [&]() {
                    const auto & props = get_data_type_props(type);
                    // Convert fixed length data types to variable
                    // size data types.
                    if (not props.is_variable_size()) {
                        type      = props.corresponding_varsize_type;
                        type_size = props.length.fixed;
                        return get_data_type_props(type);
                    }
                    return props;
                }();

                tds_ctx.write_le(static_cast<tdsl::uint8_t>(type)); // type

                auto maybe_write_collation = [&]() {
                    if (dprops.flags.has_collation) {
                        // put collation data as well
                        // FIXME: Put proper collation data!
                        tds_ctx.write_le(tdsl::uint32_t{0});
                        tds_ctx.write_le(tdsl::uint8_t{0});
                    }
                };

                switch (dprops.size_type) {
                    case e_tds_data_size_type::fixed:
                        // Do nothing.
                        break;
                    case e_tds_data_size_type::var_u8:
                        tds_ctx.write_le(
                            static_cast<tdsl::uint8_t>(type_size)); // max length - 1 byte
                        maybe_write_collation();
                        tds_ctx.write_le(static_cast<tdsl::uint8_t>(param.value.size_bytes()));
                        break;
                    case e_tds_data_size_type::var_u16:
                        tds_ctx.write_le(
                            static_cast<tdsl::uint16_t>(type_size)); // max length - 2 bytes
                        maybe_write_collation();
                        tds_ctx.write_le(static_cast<tdsl::uint16_t>(param.value.size_bytes()));
                        break;
                    case e_tds_data_size_type::var_u32:
                        tds_ctx.write_le(type_size); // max length - 2 bytes
                        maybe_write_collation();
                        tds_ctx.write_le(static_cast<tdsl::uint32_t>(param.value.size_bytes()));
                        break;
                    case e_tds_data_size_type::var_precision:
                        TDSL_NOT_YET_IMPLEMENTED;
                        break;
                    case e_tds_data_size_type::unknown:
                        TDSL_CANNOT_HAPPEN;
                        break;
                }

                if (param.value) {
                    tds_ctx.write(param.value);
                }
            }

            // Reset query state object & reassign row callback
            qstate              = {};
            qstate.row_callback = {row_callback, rcb_uptr};

            // Send the command
            tds_ctx.send_tds_pdu(e_tds_message_type::rpc);
            // Receive the response
            tds_ctx.receive_tds_pdu();
            // The state will be updated upon receiving the response
            return tdsl::uint32_t{qstate.result.affected_rows};
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

            query_result result                            = {};

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
            static constexpr auto k_min_colmetadata_bytes = 8;
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
            static constexpr auto k_colinfo_min_bytes = 6; // user_type + flags + type + colname len

            tdsl::uint16_t colindex                   = 0;

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
                            "not enough bytes to read collation information, need %d, have %zu",
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
                        TDSL_DEBUG_PRINTLN("not enough bytes to read table name, need %d, have %zu",
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
                    TDSL_DEBUG_PRINTLN("not enough bytes to read column name, need %d, have %zu",
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
                                           "field textptr, %zu more bytes needed",
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
                                       "%zu more bytes needed",
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

        // --------------------------------------------------------------------------------

        /**
         * Convert a variable type to equivalent fixed type
         *
         * @param [in] type Variable type
         * @param [in] type_size Variable type size
         *
         * @return e_tds_data_type Equivalent fixed type
         */
        TDSL_NODISCARD static inline auto var_to_fixed(e_tds_data_type type,
                                                       tdsl::size_t type_size) noexcept
            -> e_tds_data_type {

            switch (type) {
                    // Translate INTNTYPE decls to corresponding
                    // fixed length decls
                case e_tds_data_type::INTNTYPE: {
                    // For INTNTYPE, the only valid lengths are 0x01, 0x02,
                    // 0x04, and 0x08, which map to tinyint, smallint,
                    // int, and bigint SQL data types respectively.
                    switch (type_size) {
                        case 1:
                            return e_tds_data_type::INT1TYPE;
                        case 2:
                            return e_tds_data_type::INT2TYPE;
                        case 4:
                            return e_tds_data_type::INT4TYPE;
                        case 8:
                            return e_tds_data_type::INT8TYPE;
                    }
                } break;
                case e_tds_data_type::FLTNTYPE: {
                    // For FLTNTYPE, the only valid lengths are 0x04 and 0x08
                    switch (type_size) {
                        case 4:
                            return e_tds_data_type::FLT4TYPE;
                            break;
                        case 8:
                            return e_tds_data_type::FLT8TYPE;
                    }
                } break;
                case e_tds_data_type::DATETIMNTYPE: {
                    // For DATETIMNTYPE, the only valid lengths are 0x04 and 0x08,
                    // which map to smalldatetime and datetime SQL data types
                    // respectively.
                    switch (type_size) {
                        case 4:
                            return e_tds_data_type::DATETIM4TYPE;
                            break;
                        case 8:
                            return e_tds_data_type::DATETIMETYPE;
                    }
                } break;
                case e_tds_data_type::MONEYNTYPE: {
                    // For MONEYNTYPE, the only valid lengths are 0x04 and 0x08,
                    // which map to smallmoney and money SQL data types respectively.
                    switch (type_size) {
                        case 4:
                            return e_tds_data_type::MONEY4TYPE;
                            break;
                        case 8:
                            return e_tds_data_type::MONEYTYPE;
                    }
                } break;
                default:
                    return type;
            }
            TDSL_CANNOT_HAPPEN;
        }

        // --------------------------------------------------------------------------------

        /**
         * Write parameter type string
         *
         * @param [in] pb Parameter binding
         * @param [in] wc Target counted writer
         */
        static inline void
        write_param_type_str(const sql_parameter_binding & pb,
                             typename string_writer_type::counted_writer & wc) noexcept {
            /**
             * Write string representation of the type @ref pb.type
             */
            switch (var_to_fixed(pb.type, pb.type_size)) {
                case e_tds_data_type::BITTYPE:
                    wc.write("BIT");
                    break;
                case e_tds_data_type::INT1TYPE:
                    wc.write("TINYINT");
                    break;
                case e_tds_data_type::INT2TYPE:
                    wc.write("SMALLINT");
                    break;
                case e_tds_data_type::INT4TYPE:
                    wc.write("INT");
                    break;
                case e_tds_data_type::INT8TYPE:
                    wc.write("BIGINT");
                    break;
                case e_tds_data_type::NVARCHARTYPE:
                    wc.write("N");
                    TDSL_FALLTHROUGH;
                case e_tds_data_type::BIGVARCHRTYPE:
                    wc.write("VARCHAR");
                    break;
                case e_tds_data_type::FLT4TYPE:
                    wc.write("REAL");
                    break;
                case e_tds_data_type::FLT8TYPE:
                    wc.write("FLOAT");
                    break;
                case e_tds_data_type::DATETIM4TYPE:
                    wc.write("SMALL");
                    TDSL_FALLTHROUGH;
                case e_tds_data_type::DATETIMETYPE:
                    wc.write("DATETIME");
                    break;
                case e_tds_data_type::GUIDTYPE:
                    wc.write("UNIQUEIDENTIFIER");
                    break;
                case e_tds_data_type::NCHARTYPE:
                    wc.write("N");
                    TDSL_FALLTHROUGH;
                case e_tds_data_type::BIGCHARTYPE:
                    wc.write("CHAR");
                    break;
                case e_tds_data_type::BIGVARBINTYPE:
                    wc.write("VAR");
                    TDSL_FALLTHROUGH;
                case e_tds_data_type::BIGBINARYTYPE:
                    wc.write("BINARY");
                    break;
                case e_tds_data_type::INTNTYPE:
                case e_tds_data_type::FLTNTYPE:
                case e_tds_data_type::DATETIMNTYPE:
                case e_tds_data_type::MONEYNTYPE:
                case e_tds_data_type::DECIMALNTYPE:
                case e_tds_data_type::NUMERICNTYPE:
                    TDSL_CANNOT_HAPPEN;
                    break;
                default:
                    TDSL_NOT_YET_IMPLEMENTED;
                    break;
            }
        };

        // --------------------------------------------------------------------------------

        /**
         * Write parameter binding @p pb's length to @p wc
         * if applicable
         *
         * @param [in] pb Parameter binding
         * @param [in] wc Target counted writer
         */
        static inline void
        write_param_len_str(const sql_parameter_binding & pb,
                            typename string_writer_type::counted_writer & wc) noexcept {
            auto write_explicit_length = [&wc](tdsl::size_t len) {
                char utos_buf [10] = {0};
                wc.write("(");
                wc.write(tdsl::string_view{tdsl::utos(len, utos_buf)});
                wc.write(")");
            };

            switch (pb.type) {
                case e_tds_data_type::BIGVARBINTYPE: // varbinary
                case e_tds_data_type::BIGBINARYTYPE: // binary
                case e_tds_data_type::BIGVARCHRTYPE: // varchar
                case e_tds_data_type::BIGCHARTYPE:   // char(N)
                {
                    // For char types, use data length if user not specified a length
                    // explicitly. Otherwise, respect specified length.
                    write_explicit_length(pb.type_size ? pb.type_size : pb.value.size_bytes());
                } break;
                case e_tds_data_type::NVARCHARTYPE: // nvarchar
                case e_tds_data_type::NCHARTYPE:    // nchar(N)
                {
                    // For char types, use data length if user not specified a length
                    // explicitly. Otherwise, respect specified length.
                    write_explicit_length(
                        pb.type_size ? pb.type_size : (pb.value.size_bytes() / sizeof(char16_t)));
                } break;
                default:
                    break;
            }
        };
    };
}} // namespace tdsl::detail

#endif