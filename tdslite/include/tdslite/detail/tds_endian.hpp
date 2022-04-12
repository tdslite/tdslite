/**
 * ____________________________________________________
 * Endianness related definitions
 *
 * @file   tds_endianness.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSLITE_DETAIL_TDS_ENDIAN_HPP
#define TDSLITE_DETAIL_TDS_ENDIAN_HPP

namespace tdslite {

    /**
     * Endianness kinds
     */
    enum class endian
    {
#ifdef _WIN32
        little     = 0,
        big        = 1,
        native     = little,
        non_native = big
#else
        little     = __ORDER_LITTLE_ENDIAN__,
        big        = __ORDER_BIG_ENDIAN__,
        native     = __BYTE_ORDER__,
        non_native = (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ ? __ORDER_BIG_ENDIAN__ : __ORDER_LITTLE_ENDIAN__)
#endif
    };

} // namespace tdslite

#endif