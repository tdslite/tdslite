/**
 * ____________________________________________________
 * sql_smalldatetime data type
 *
 * @file   sql_smalldatetime.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_SQLTYPES_SQLSMALLDATETIME_HPP
#define TDSL_DETAIL_SQLTYPES_SQLSMALLDATETIME_HPP

#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/detail/tdsl_tds_column_info.hpp>
#include <tdslite/detail/sqltypes/sql_type_base.hpp>

namespace tdsl {

    /**
     * smalldatetime sql type
     */
    struct sql_smalldatetime : public sql_type_base {

        // --------------------------------------------------------------------------------

        /**
         * Construct a new SQL smalldatetime object
         *
         * @param [in] v View to bytes to be interpreted as sql_smalldatetime
         */
        inline explicit sql_smalldatetime(tdsl::byte_view v,
                                          const tdsl::tds_column_info & col) noexcept {
            (void) col;
            TDSL_ASSERT(v.size_bytes() == (sizeof(tdsl::uint16_t) * 2));
            tdsl::binary_reader<tdsl::endian::little> br{v};
            days_elapsed    = br.read<tdsl::uint16_t>();
            minutes_elapsed = br.read<tdsl::uint16_t>();
        }

        // --------------------------------------------------------------------------------

        /**
         * Convert smalldatetime to unix timestamp
         *
         * @return tdsl::uint64_t smalldatetime value as unix timestamp
         */
        inline tdsl::uint64_t to_unix_timestamp() const noexcept {
            constexpr auto days_between_epochs = ((1970 - 1900) * 365);
            if (days_elapsed < days_between_epochs) {
                return 0; // 1-1-1970
            }
            return ((days_elapsed - days_between_epochs) * 86400) + (minutes_elapsed * 60);
        }

        // --------------------------------------------------------------------------------

        // One 2-byte unsigned integer that represents the
        // number of days since January 1, 1900.
        tdsl::uint16_t days_elapsed;
        // One 2-byte unsigned integer that represents the
        // number of minutes elapsed since 12 AM that day.
        tdsl::uint16_t minutes_elapsed;
    };

} // namespace tdsl

#endif