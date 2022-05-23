/**
 * _________________________________________________
 *
 * @file   tdsl_driver.hpp
 * @author Mustafa Kemal GILOR <mgilor@nettsi.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/detail/tdsl_tds_context.hpp>
#include <tdslite/detail/tdsl_login_context.hpp>
#include <tdslite/detail/tdsl_command_context.hpp>

namespace tdsl { namespace detail {

    /**
     * tds-lite driver
     *
     * @tparam NetImpl The networking implementation type
     */
    template <typename NetImpl>
    struct tdsl_driver {

        using tds_context_type   = tds_context<NetImpl>;
        using login_context_type = login_context<NetImpl>;
        using sql_command        = command_context<NetImpl>;

        // register_connection_state_callback

        void login();
        void logout();

        template <typename T>
        void execute_non_query(T command) const noexcept {
            sql_command{tds_ctx}.execute_non_query(command);
        }

    private:
        tds_context<NetImpl> tds_ctx;
    };

}} // namespace tdsl::detail