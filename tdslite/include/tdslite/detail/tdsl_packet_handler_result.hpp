/**
 * _________________________________________________
 *
 * @file   tdsl_packet_handler_result.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   03.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl {
    template <typename Status, Status DefaultStatusV>
    struct packet_handler_result {
        Status status                 = {DefaultStatusV};
        tdsl::uint32_t consumed_bytes = {0};
        tdsl::uint32_t needed_bytes   = {0};
    };
} // namespace tdsl