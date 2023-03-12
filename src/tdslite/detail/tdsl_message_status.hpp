/**
 * ____________________________________________________
 *
 * @file   tds_message_status.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   18.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TDS_MESSAGE_STATUS_HPP
#define TDSL_DETAIL_TDS_MESSAGE_STATUS_HPP

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_endian.hpp>

namespace tdsl { namespace detail {

    /**
     * TDS protocol message status
     */
    struct tds_message_status {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        tdsl::uint8_t end_of_message : 1;
        tdsl::uint8_t ignore_this_event : 1;
        tdsl::uint8_t event_notification : 1;
        tdsl::uint8_t reset_connection : 1;
        tdsl::uint8_t reset_connection_skip_tran : 1;
        tdsl::uint8_t reserved : 3;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        tdsl::uint8_t reserved : 3;
        tdsl::uint8_t reset_connection_skip_tran : 1;
        tdsl::uint8_t reset_connection : 1;
        tdsl::uint8_t event_notification : 1;
        tdsl::uint8_t ignore_this_event : 1;
        tdsl::uint8_t end_of_message : 1;
#else
#error "undefined endianness!"
#endif
    };
}} // namespace tdsl::detail

#endif