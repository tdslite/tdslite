/**
 * ____________________________________________________
 * Result of sending a TDS message
 *
 * @file   tdsl_send_status.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   26.11.2026
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_SEND_STATUS_HPP
#define TDSL_DETAIL_SEND_STATUS_HPP

#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl { namespace detail {

    /**
     * Result of sending the message built in the network buffer
     */
    enum class e_send_status : tdsl::uint8_t
    {
        // The message is sent
        success           = 0,
        // The message does not fit into the network buffer. Nothing is sent.
        request_too_large = 1,
        // Sending the message failed (e.g. disconnected). The message
        // might have been sent partially.
        network_error     = 2,
    };

}} // namespace tdsl::detail

#endif
