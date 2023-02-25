/**
 * ____________________________________________________
 *
 * @file   tdsl_packet_handler_result.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   03.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_PACKET_HANDLER_RESULT_HPP
#define TDSL_DETAIL_PACKET_HANDLER_RESULT_HPP

#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl {
    template <typename Status, Status DefaultStatusV>
    struct packet_handler_result {
        Status status             = {DefaultStatusV};
        /**
         * `needed_bytes` is a way to signal network layer
         * to know at least how many bytes more are needed
         * to complete reading a part of message, which size
         * or expected amount is already known. this allows
         * networking code to reduce the unnecessary back
         * and forth between parsing code and receive function.
         *
         * In order to needed_bytes to be meaningful, the network
         * implementation has to support it via constructs such as
         * read(transfer_at_least{needed_bytes}).
         */
        tdsl::size_t needed_bytes = {0};
    };
} // namespace tdsl

#endif