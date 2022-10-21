/**
 * _________________________________________________
 *
 * @file   tdsl_driver.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSL_DETAIL_DRIVER_HPP
#define TDSL_DETAIL_DRIVER_HPP

#include <tdslite/detail/tdsl_tds_context.hpp>
#include <tdslite/detail/tdsl_login_context.hpp>
#include <tdslite/detail/tdsl_command_context.hpp>

namespace tdsl { namespace detail {

    /**
     * tdslite TDS driver
     *
     * @tparam NetImpl The networking implementation
     */
    template <typename NetImpl>
    struct tdsl_driver {
        using tds_context_type       = detail::tds_context<NetImpl>;
        using login_context_type     = detail::login_context<NetImpl>;
        using login_parameters_type  = typename login_context_type::login_parameters;
        using wlogin_parameters_type = typename login_context_type::wlogin_parameters;
        using sql_command_type       = detail::command_context<NetImpl>;
        using row_type               = tdsl_row;

        struct connection_parameters : public login_parameters_type {
            tdsl::uint16_t port = {1433};
        };

        // TODO: Implement this
        inline auto connect(tdsl::string_view connection_string) -> int;
        // TODO: Implement this?
        void logout();

        // --------------------------------------------------------------------------------

        // connect & login here
        inline auto connect(const connection_parameters & p) -> int {
            auto result = tds_ctx.connect(p.server_name, p.port);
            if (not(result == 0)) {
                return result;
            }
            login_context_type{tds_ctx}.do_login(p);
            return result;
        }

        // --------------------------------------------------------------------------------

        void login(const wlogin_parameters_type & p) {
            login_context_type{tds_ctx}.do_login(p);
        }

        // --------------------------------------------------------------------------------

        /**
         *
         *
         */
        void
        set_info_callback(void * user_ptr,
                          typename tds_context_type::info_callback_type::function_type callback) {
            tds_ctx.callbacks.info = {user_ptr, callback};
        }

        // --------------------------------------------------------------------------------

        template <typename T>
        inline auto execute_query(T command) noexcept -> tdsl::uint32_t {
            TDSL_ASSERT(tds_ctx.is_authenticated());
            return sql_command_type{tds_ctx}.execute_query(command);
        }

        // --------------------------------------------------------------------------------

        template <typename T>
        inline auto execute_query(T command, void * uptr,
                                  void (*row_callback)(void *, const tds_colmetadata_token &,
                                                       const tdsl_row &)) noexcept
            -> tdsl::uint32_t {
            TDSL_ASSERT(tds_ctx.is_authenticated());
            return sql_command_type{tds_ctx}.execute_query(command, uptr, row_callback);
        }

    private:
        tds_context_type tds_ctx;
    };
}} // namespace tdsl::detail

#endif