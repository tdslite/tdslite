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

#include <tdslite/detail/tds_inttypes.hpp>

namespace tdslite {

    /**
     * TDS protocol message status
     */
    struct tds_message_status {
        tdslite::uint8_t end_of_message : 1;
        tdslite::uint8_t ignore_this_event : 1;
        tdslite::uint8_t event_notification : 1;
        tdslite::uint8_t reset_connection : 1;
        tdslite::uint8_t reset_connection_skip_tran : 1;
        tdslite::uint8_t reserved : 3;
    };

} // namespace tdslite