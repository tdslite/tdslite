/**
 * _________________________________________________
 *
 * @file   tds_message_status.hpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   18.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl { namespace detail {

    /**
     * TDS protocol message status
     */
    struct tds_message_status {
        tdsl::uint8_t end_of_message : 1;
        tdsl::uint8_t ignore_this_event : 1;
        tdsl::uint8_t event_notification : 1;
        tdsl::uint8_t reset_connection : 1;
        tdsl::uint8_t reset_connection_skip_tran : 1;
        tdsl::uint8_t reserved : 3;
    };
}} // namespace tdsl::detail