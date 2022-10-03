/**
 * _________________________________________________
 *
 * @file   tdsl_token_handler_result.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   03.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/detail/tdsl_packet_handler_result.hpp>

namespace tdsl {

    enum class token_handler_status
    {
        success                   = 0,
        unhandled                 = -1,
        not_enough_bytes          = -2,
        not_enough_memory         = -3,
        unknown_column_size_type  = -4,
        missing_prior_colmetadata = -5
    };

    using token_handler_result = packet_handler_result<token_handler_status, token_handler_status::unhandled>;

} // namespace tdsl