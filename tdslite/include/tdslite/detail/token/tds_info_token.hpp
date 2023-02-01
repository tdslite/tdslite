/**
 * ____________________________________________________
 *
 * @file   tds_info_token.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   22.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TOKEN_TDS_INFO_TOKEN_HPP
#define TDSL_DETAIL_TOKEN_TDS_INFO_TOKEN_HPP

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl {
    struct tds_info_token {
        tdsl::uint32_t number                  = {0};
        tdsl::uint8_t state                    = {0};
        tdsl::uint8_t class_                   = {0};
        tdsl::uint16_t line_number             = {0};
        tdsl::span<const char16_t> msgtext     = {};
        tdsl::span<const char16_t> server_name = {};
        tdsl::span<const char16_t> proc_name   = {};

        // --------------------------------------------------------------------------------

        /**
         * Check whether the token is an ERROR token or not
         *
         * @return true Token is an ERROR token
         * @return false Token is an INFO token
         */
        inline auto is_error() const noexcept -> bool {
            return class_ > 10;
        }

        // --------------------------------------------------------------------------------

        /**
         * Check whether the token is an INFO token or not
         *
         * @return true Token is an INFO token
         * @return false Token is an ERROR token
         */
        inline auto is_info() const noexcept -> bool {
            return !is_error();
        }
    };
} // namespace tdsl

#endif