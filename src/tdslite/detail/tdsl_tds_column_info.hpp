/**
 * ____________________________________________________
 *
 * @file   tdsl_tds_column_info.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   09.08.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TDSL_TDS_COLUMN_INFO
#define TDSL_DETAIL_TDSL_TDS_COLUMN_INFO

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/detail/tdsl_data_type.hpp>

namespace tdsl {

    // Packed into 12-byte layout to make
    // most of the space useful.
    struct tds_column_info {
        /* User-defined type value */
        tdsl::uint16_t user_type              = {0};
        tdsl::uint16_t flags                  = {0};
        /* Data type of the column */
        detail::e_tds_data_type type          = {static_cast<detail::e_tds_data_type>(0)};
        /* Length of the name of the column */
        tdsl::uint8_t colname_length_in_chars = {0};

        union {
            struct {
                tdsl::uint32_t length;
            } u32l; // types with variable length of unsigned 32-bit size

            struct {
                tdsl::uint16_t length;
                tdsl::uint8_t _unused [2];
            } u16l; // types with variable length of unsigned 16-bit size

            struct {
                tdsl::uint8_t length;
                tdsl::uint8_t _unused [3];
            } u8l; // types with variable length of unsigned 8-bit size

            struct {
                tdsl::uint8_t length; // this is the actual length, not the length's length.
                tdsl::uint8_t _unused [3];
            } fixed; // types with fixed length

            struct {
                tdsl::uint8_t length;
                tdsl::uint8_t precision;
                tdsl::uint8_t scale;
                tdsl::uint8_t _unused [1];
            } ps = {};    // types with precision and scale
        } typeprops = {}; // type-specific properties
    };
} // namespace tdsl

#endif