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
#include <tdslite/detail/tdsl_callback_context.hpp>
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
        using string_writer_type       = string_parameter_writer<tds_context_type>;
        using nonquery_result_callback = callback_context<tds_done_token>;

        /**
         * Default c-tor
         *
         * @param [in] ctx The TDS context associated with the command
         */
        command_context(tds_context_type & ctx) : tds_ctx(ctx) {
            tds_ctx.do_register_info_token_callback(
                this, +[](void *, const tds_info_token &) -> tdsl::uint32_t {
                    return 0;
                });
            tds_ctx.do_register_done_token_callback(this, [](void * uptr, const tds_done_token & dt) -> tdsl::uint32_t {
                command_context & ctx  = *static_cast<command_context *>(uptr);
                ctx.last_affected_rows = dt.done_row_count;
                TDSLITE_DEBUG_PRINT("received done token %d\n", dt.done_row_count);
                return 0;
            });
        }

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
            typename tds_context_type::row_callback_fn_t row_callback = +[](void *, const tds_colmetadata_token &, const tdsl_row &) {
            }) const noexcept {

            tds_ctx.do_register_row_callback(rcb_uptr, row_callback);

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
            tds_ctx.recv(8);
            return last_affected_rows;
        }

    private:
        tds_context_type & tds_ctx;
        tdsl::uint32_t last_affected_rows;
    };
}} // namespace tdsl::detail