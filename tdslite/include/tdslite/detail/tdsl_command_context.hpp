/**
 * _________________________________________________
 *
 * @file   tdsl_query_context.hpp
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
#include <tdslite/detail/tdsl_done_token.hpp>

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
        }

        /**
         * Execute a command that is a non-query type, meaning the command
         * will not result in a result set (e.g. INSERT, UPDATE, DELETE).
         *
         * @tparam T Auto-deduced string type (char_span or u16char_span)
         *
         * @param [in] command SQL command to execute
         * @param [in] callback Callback function to execute on command result
         */
        template <typename T, traits::enable_if_same_any<T, string_view, wstring_view> = true>
        inline void execute_non_query(T command) const noexcept {
            // Write the TDS header for the command
            tds_ctx.write_tds_header(e_tds_message_type::sql_batch);
            // Write the SQL command
            string_writer_type::write(tds_ctx, command);
            // Put the data length (size of the SQL command)
            tds_ctx.put_tds_header_length(string_writer_type::calculate_write_size(command));
            // Send the command
            tds_ctx.send();
        }

    private:
        tds_context_type & tds_ctx;
    };
}} // namespace tdsl::detail