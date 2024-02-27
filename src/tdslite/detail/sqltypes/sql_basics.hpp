/**
 * ____________________________________________________
 * Basic SQL data types
 *
 * @file   sql_basics.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_SQLTYPES_SQL_BASICS_HPP
#define TDSL_DETAIL_SQLTYPES_SQL_BASICS_HPP

#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl {
    using sql_bit      = bool;
    using sql_tinyint  = tdsl::uint8_t;
    using sql_smallint = tdsl::int16_t;
    using sql_int      = tdsl::int32_t;
    using sql_bigint   = tdsl::int64_t;
    using sql_float4   = float;
    using sql_float8   = double;
} // namespace tdsl

#endif