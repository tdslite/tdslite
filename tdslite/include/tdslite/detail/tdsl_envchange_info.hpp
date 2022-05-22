/**
 * _________________________________________________
 *
 * @file   tdsl_envchange_data.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   22.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/detail/tdsl_envchange_type.hpp>
#include <tdslite/util/tdsl_string_view.hpp>

namespace tdsl {
    struct tds_envchange_info {
        detail::e_tds_envchange_type type; // Environment change type
        tdsl::u16char_span new_value;      // New value (if applicable)
        tdsl::u16char_span old_value;      // Previous value (if applicable)
    };
} // namespace tdsl