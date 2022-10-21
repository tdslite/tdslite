/**
 * _________________________________________________
 *
 * @file   tds_loginack_token.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   23.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSL_DETAIL_TOKEN_TDS_LOGINACK_TOKEN_HPP
#define TDSL_DETAIL_TOKEN_TDS_LOGINACK_TOKEN_HPP

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_span.hpp>

namespace tdsl {
    struct tds_login_ack_token {

        /**
         * The type of interface with which the server will accept the client requests.
         *
         * 0 -> SQL_DFLT (server confirms that whatever is sent by the client is acceptable.
         * If the client requested SQL_DFLT, SQL_TSQL will be used).
         * 1 -> SQL_TSQL (TSQL is accepted)
         */
        tdsl::uint8_t interface      = {0};

        /**
         * The TDS version being used by the server
         */
        tdsl::uint32_t tds_version   = {0};

        /**
         * The name of the server
         */
        tdsl::u16char_view prog_name = {};

        // The server version
        struct {
            // The major version number
            tdsl::uint8_t maj         = {0};
            // The minor version number
            tdsl::uint8_t min         = {0};
            // The high byte of the build number
            tdsl::uint8_t buildnum_hi = {0};
            // The low byte of the build number
            tdsl::uint8_t buildnum_lo = {0};
        } prog_version = {};
    };
} // namespace tdsl

#endif