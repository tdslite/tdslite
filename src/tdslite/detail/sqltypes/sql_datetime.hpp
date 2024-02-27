/**
 * ____________________________________________________
 * sql_datetime data type
 *
 * @file   sql_datetime.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_SQLTYPES_SQLDATETIME_HPP
#define TDSL_DETAIL_SQLTYPES_SQLDATETIME_HPP

#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/detail/sqltypes/sql_type_base.hpp>
#include <tdslite/detail/tdsl_tds_column_info.hpp>

namespace tdsl {

    /**
     * datetime sql type
     */
    struct sql_datetime : public sql_type_base {

        // --------------------------------------------------------------------------------

        /**
         * Construct a new SQL smalldatetime object
         *
         * @param [in] v View to bytes to be interpreted as sql_smalldatetime
         */
        inline explicit sql_datetime(tdsl::byte_view v,
                                     const tdsl::tds_column_info & col) noexcept {
            (void) col;
            TDSL_ASSERT(v.size_bytes() == (sizeof(tdsl::int32_t) + sizeof(uint32_t)));
            tdsl::binary_reader<tdsl::endian::little> br{v};
            days_elapsed         = br.read<tdsl::int32_t>();
            centiseconds_elapsed = br.read<tdsl::uint32_t>();
        }

        // --------------------------------------------------------------------------------

        /**
         * Convert datetime to unix timestamp
         *
         * @return tdsl::uint64_t datetime value as unix timestamp
         */
        inline tdsl::uint64_t to_unix_timestamp() const noexcept {
            constexpr auto days_between_epochs = ((1970 - 1900) * 365);
            if (days_elapsed < days_between_epochs) {
                return 0; // 1-1-1970
            }
            return ((days_elapsed - days_between_epochs) * 86400ul) + (centiseconds_elapsed / 100);
        }

        // --------------------------------------------------------------------------------

        // One 4-byte signed integer that represents the number of days
        // since January 1, 1900. Negative numbers are allowed to represent
        // dates since January 1, 1753.
        tdsl::int32_t days_elapsed;
        // One 4-byte unsigned integer that represents the number of one
        // three-hundredths of a second (300 counts per second) elapsed
        // since 12 AM that day.
        tdsl::uint32_t centiseconds_elapsed;
    };

} // namespace tdsl

#endif