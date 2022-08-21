/**
 * _________________________________________________
 *
 * @file   tdsl_loginack_token.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   23.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

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
        tdsl::uint8_t interface;
        /**
         * The TDS version being used by the server
         */
        tdsl::uint32_t tds_version;

        /**
         * The name of the server
         */
        tdsl::u16char_span prog_name;

        // The server version
        struct {
            // The major version number
            tdsl::uint8_t maj;
            // The minor version number
            tdsl::uint8_t min;
            // The high byte of the build number
            tdsl::uint8_t buildnum_hi;
            // The low byte of the build number
            tdsl::uint8_t buildnum_lo;
        } prog_version;

        // prog_version -> maj.min.hi.lo
    };
} // namespace tdsl