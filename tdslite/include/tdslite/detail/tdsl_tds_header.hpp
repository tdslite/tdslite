/**
 * _________________________________________________
 *
 * @file   tdsl_tds_header.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   19.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>

namespace tdsl { namespace detail {
    /**
     * The tabular data stream protocol header
     */
    struct tds_header {
        tdsl::uint8_t type;
        tdsl::uint8_t status;
        tdsl::uint16_t length;
        tdsl::uint16_t channel;
        tdsl::uint8_t packet_number;
        tdsl::uint8_t window;
    } TDSL_PACKED;
}} // namespace tdsl::detail