/**
 * _______________________________________________________
 * C++ aliases for integer types
 *
 * @file   tds_inttypes.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _______________________________________________________
 */

#include <stdint.h>

#ifndef TDSLITE_DETAIL_TDS_INTTYPES_HPP
#define TDSLITE_DETAIL_TDS_INTTYPES_HPP

namespace tdslite {
    // *_be_* = type alias for denoting a big-endian integer
    // *_le_* = type alias for denoting a little-endian integer

    using ::int16_t;
    using int16_be_t = tdslite::int16_t;
    using int16_le_t = tdslite::int16_t;
    using ::int32_t;
    using int32_be_t = tdslite::int32_t;
    using int32_le_t = tdslite::int32_t;
    using ::int64_t;
    using int64_be_t = tdslite::int64_t;
    using int64_le_t = tdslite::int64_t;
    using ::int8_t;

    using ::int_fast16_t;
    using ::int_fast32_t;
    using ::int_fast64_t;
    using ::int_fast8_t;

    using ::int_least16_t;
    using ::int_least32_t;
    using ::int_least64_t;
    using ::int_least8_t;

    using ::intmax_t;
    using ::intptr_t;

    using ::uint16_t;
    using uint16_be_t = tdslite::uint16_t;
    using uint16_le_t = tdslite::uint16_t;
    using ::uint32_t;
    using uint32_be_t = tdslite::uint32_t;
    using uint32_le_t = tdslite::uint32_t;
    using ::uint64_t;
    using uint64_be_t = tdslite::uint64_t;
    using uint64_le_t = tdslite::uint64_t;
    using ::uint8_t;

    using ::uint_fast16_t;
    using ::uint_fast32_t;
    using ::uint_fast64_t;
    using ::uint_fast8_t;

    using ::uint_least16_t;
    using ::uint_least32_t;
    using ::uint_least64_t;
    using ::uint_least8_t;

    using ::uintmax_t;
    using ::uintptr_t;
} // namespace tdslite

// user-defined literals for casting integers

inline static constexpr tdslite::uint8_t operator"" _tdsu8(unsigned long long arg) noexcept {
    return static_cast<tdslite::uint8_t>(arg);
}

inline static constexpr tdslite::int8_t operator"" _tdsi8(unsigned long long arg) noexcept {
    return static_cast<tdslite::int8_t>(arg);
}

inline static constexpr tdslite::uint16_t operator"" _tdsu16(unsigned long long arg) noexcept {
    return static_cast<tdslite::uint16_t>(arg);
}

inline static constexpr tdslite::int16_t operator"" _tdsi16(unsigned long long arg) noexcept {
    return static_cast<tdslite::int16_t>(arg);
}

inline static constexpr tdslite::uint32_t operator"" _tdsu32(unsigned long long arg) noexcept {
    return static_cast<tdslite::uint32_t>(arg);
}

inline static constexpr tdslite::int32_t operator"" _tdsi32(unsigned long long arg) noexcept {
    return static_cast<tdslite::int32_t>(arg);
}

inline static constexpr tdslite::uint64_t operator"" _tdsu64(unsigned long long arg) noexcept {
    return static_cast<tdslite::uint64_t>(arg);
}

inline static constexpr tdslite::int64_t operator"" _tdsi64(unsigned long long arg) noexcept {
    return static_cast<tdslite::int64_t>(arg);
}

#endif