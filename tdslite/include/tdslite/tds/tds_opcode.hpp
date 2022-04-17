/**
 * _________________________________________________
 *
 * @file   tds_opcode.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   17.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/detail/tds_inttypes.hpp>

namespace tdslite {
    /**
     * TDS protocol opcodes
     */
    enum class e_tds_opcode : tdslite::uint8_t
    { login = 0x10 };
} // namespace tdslite