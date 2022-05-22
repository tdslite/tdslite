/**
 * _________________________________________________
 *
 * @file   tdsl_info_msg.hpp
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
    struct tds_info_msg {

        inline auto is_error() const -> bool {
            return class_ > 10;
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