/**
 * _________________________________________________
 *
 * @file   tds_envchange_token.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   22.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSL_DETAIL_TOKEN_TDS_ENVCHANGE_TOKEN_HPP
#define TDSL_DETAIL_TOKEN_TDS_ENVCHANGE_TOKEN_HPP

#include <tdslite/detail/tdsl_envchange_type.hpp>
#include <tdslite/util/tdsl_span.hpp>

namespace tdsl {
    /**
     * Environment change token
     */
    struct tds_envchange_token {
        // Environment change type
        detail::e_tds_envchange_type type = {static_cast<detail::e_tds_envchange_type>(0)};
        // New value (if applicable)
        tdsl::u16char_view new_value      = {};
        // Previous value (if applicable)
        tdsl::u16char_view old_value      = {};
    };
} // namespace tdsl

#endif