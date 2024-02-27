/**
 * ____________________________________________________
 * sql_money data type
 *
 * @file   sql_money.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_SQLTYPES_SQLMONEY_HPP
#define TDSL_DETAIL_SQLTYPES_SQLMONEY_HPP

#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/detail/tdsl_tds_column_info.hpp>
#include <tdslite/detail/sqltypes/sql_type_base.hpp>

namespace tdsl {

    /**
     * money sql type
     */
    struct sql_money : public sql_type_base {

        /**
         * Construct a new sql money object
         *
         * @param [in] v View to bytes to be interpreted as sql_money
         */
        inline explicit sql_money(tdsl::byte_view v, const tdsl::tds_column_info & col) noexcept {
            (void) col;
            TDSL_ASSERT(v.size_bytes() == (sizeof(tdsl::uint32_t) * 2));
            // money is represented as an 8-byte signed integer. The TDS value is the money
            // value multiplied by 10^4. The 8-byte signed integer itself is represented in the
            // following sequence:
            // * One 4-byte integer that represents the more significant half.
            // * One 4-byte integer that represents the less significant half.
            tdsl::binary_reader<tdsl::endian::little> br{v};
            const tdsl::uint32_t msh = br.read<tdsl::uint32_t>();
            const tdsl::uint32_t lsh = br.read<tdsl::uint32_t>();
            value = static_cast<tdsl::int64_t>((static_cast<tdsl::uint64_t>(msh) << 32) |
                                               (static_cast<tdsl::uint64_t>(lsh)));
        }

        // --------------------------------------------------------------------------------

        /**
         * Calculate the integer part
         *
         * @return tdsl::int64_t sql_money integer part
         */
        inline TDSL_NODISCARD tdsl::int64_t integer() const noexcept {
            return value / 10000;
        }

        // --------------------------------------------------------------------------------

        /**
         * Calculate the fraction part
         *
         * @return tdsl::int64_t sql_money fraction part
         */
        inline TDSL_NODISCARD tdsl::int64_t fraction() const noexcept {
            return value % 10000;
        }

        // --------------------------------------------------------------------------------

        /**
         * Raw 8 byte integer stored in the database
         */
        inline TDSL_NODISCARD tdsl::int64_t raw() const noexcept {
            return value;
        }

    private:
        tdsl::int64_t value;
    };
} // namespace tdsl

#endif