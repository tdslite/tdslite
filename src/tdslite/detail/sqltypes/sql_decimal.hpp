/**
 * ____________________________________________________
 * sql_decimal(/sql_numeric) data type
 *
 * @file   sql_decimal.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_SQLTYPES_SQL_DECIMAL_HPP
#define TDSL_DETAIL_SQLTYPES_SQL_DECIMAL_HPP

#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/detail/tdsl_tds_column_info.hpp>
#include <tdslite/detail/sqltypes/sql_type_base.hpp>

namespace tdsl {

    /**
     * decimal sql type
     */
    struct sql_decimal : public sql_type_base {

        /**
         * Construct a new sql money object
         *
         * @param [in] v View to bytes to be interpreted as sql_decimal
         */
        inline explicit sql_decimal(tdsl::byte_view v, const tdsl::tds_column_info & col) noexcept {

            TDSL_ASSERT(stor.value == 0);
            TDSL_ASSERT(flags.precision == 0);
            TDSL_ASSERT(flags.scale == 0);
            TDSL_ASSERT(flags.sign == 0);

            // Decimal or Numeric is defined as decimal(p, s) or numeric(p, s), where p is the
            // precision and s is the scale. The value is represented in the following sequence:
            //  * One 1-byte unsigned integer that represents the sign of the decimal value as
            //  follows:
            //      * 0 means negative.
            //      * 1 means nonnegative.
            //  * One 4-, 8-, 12-, or 16-byte signed integer that represents the decimal value
            //  multiplied by 10^s.
            // The maximum size of this integer is determined based on p as follows:
            //  * 4 bytes if 1 <= p <= 9.
            //  * 8 bytes if 10 <= p <= 19.
            //  * 12 bytes if 20 <= p <= 28.
            //  * 16 bytes if 29 <= p <= 38.
            // The actual size of this integer could be less than the maximum size, depending on
            // the value. In all cases, the integer part MUST be 4, 8, 12, or 16 bytes.
            // https://github.com/microsoft/referencesource/blob/master/System.Data/System/Data/SQLTypes/SQLDecimal.cs
            flags.precision           = col.typeprops.ps.precision;
            flags.scale               = col.typeprops.ps.scale;

            // https://www.h-schmidt.net/FloatConverter/IEEE754.html
            tdsl::uint8_t read_length = 0;
            if (col.typeprops.ps.precision >= 1 && col.typeprops.ps.precision <= 9) {
                read_length = 4;
            }
            else if (col.typeprops.ps.precision >= 10 && col.typeprops.ps.precision <= 18) {
                read_length = 8;
            }
            else {
                // Precision values larger than 18 are not supported (yet),
                // the value would not fit into 64 bits.
                return;
            }

            // NULL (no data), or malformed value
            if (v.size_bytes() < (1u + read_length)) {
                return;
            }

            TDSL_ASSERT(read_length <= sizeof(stor.raw));

            tdsl::binary_reader<tdsl::endian::little> br{v};
            flags.sign                 = not(br.read<tdsl::uint8_t>() == 0);
            const tdsl::byte_view data = br.read(read_length);
            memcpy(stor.raw, data.data(), data.size_bytes());
            flags.valid = true;
        }

        // --------------------------------------------------------------------------------

        /**
         * Whether the value could be read. The value is invalid (and reads as
         * zero) if the field is NULL, its precision is larger than 18
         * (not supported), or it's malformed.
         */
        inline TDSL_NODISCARD bool is_valid() const noexcept {
            return flags.valid;
        }

        // --------------------------------------------------------------------------------

        /**
         * Integer part of the decimal
         */
        inline TDSL_NODISCARD tdsl::int64_t integer() const noexcept {
            const auto mod = modifier();
            return (stor.value / mod) * (flags.sign ? 1 : -1);
        }

        /**
         * Fraction part of the decimal
         */
        inline TDSL_NODISCARD tdsl::int64_t fraction() const noexcept {
            const auto mod = modifier();
            return (stor.value % mod) * (flags.sign ? 1 : -1);
        }

    private:
        inline TDSL_NODISCARD tdsl::int64_t modifier() const noexcept {
            tdsl::int64_t result = 1;
            for (tdsl::uint64_t i = 0; i < flags.scale; i++) {
                result *= 10;
            }
            return result;
        }

        struct flags {
            tdsl::uint8_t precision : 6; // 0/31
            bool sign : 1;               // 0 - negative, 1 - positive
            bool valid : 1;              // see is_valid()
            tdsl::uint8_t scale : 6;
            bool reserved_2 : 2;
            tdsl::uint64_t reserved_3 : 48;
        } flags = {};

        union storage {
            tdsl::int64_t value;
            tdsl::uint8_t raw [sizeof(decltype(value))];
        } stor = {};
    };

    using sql_numeric = sql_decimal;

} // namespace tdsl

#endif