/**
 * _________________________________________________
 *
 * @file   tdsl_info_token.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   22.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl {
    struct tds_info_token {

        /**
         * Check whether the token is an ERROR token or not
         *
         * @return true Token is an ERROR token
         * @return false Token is an INFO token
         */
        inline auto is_error() const -> bool {
            return class_ > 10;
        }

        /**
         * Check whether the token is an INFO token or not
         *
         * @return true Token is an INFO token
         * @return false Token is an ERROR token
         */
        inline auto is_info() const -> bool {
            return !is_error();
        }

        tdsl::uint32_t number;
        tdsl::uint8_t state;
        tdsl::uint8_t class_;
        tdsl::uint16_t line_number;
        tdsl::span<const char16_t> msgtext;
        tdsl::span<const char16_t> server_name;
        tdsl::span<const char16_t> proc_name;
    };
} // namespace tdsl