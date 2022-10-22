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

#ifndef TDSL_UTIL_INTTYPES_HPP
#define TDSL_UTIL_INTTYPES_HPP

namespace tdsl {
    // *_be_* = type alias for denoting a big-endian integer
    // *_le_* = type alias for denoting a little-endian integer

    using ::int16_t;
    using int16_be_t = tdsl::int16_t;
    using int16_le_t = tdsl::int16_t;
    using ::int32_t;
    using int32_be_t = tdsl::int32_t;
    using int32_le_t = tdsl::int32_t;
    using ::int64_t;
    using int64_be_t = tdsl::int64_t;
    using int64_le_t = tdsl::int64_t;
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
    using uint16_be_t = tdsl::uint16_t;
    using uint16_le_t = tdsl::uint16_t;
    using ::uint32_t;
    using uint32_be_t = tdsl::uint32_t;
    using uint32_le_t = tdsl::uint32_t;
    using ::uint64_t;
    using uint64_be_t = tdsl::uint64_t;
    using uint64_le_t = tdsl::uint64_t;
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

} // namespace tdsl

static_assert(sizeof(tdsl::int8_t) == 1, "sizeof(tdsl::int8_t) is not 1 in your platform!");
static_assert(sizeof(tdsl::uint8_t) == 1, "sizeof(tdsl::uint8_t) is not 1 in your platform!");
static_assert(sizeof(tdsl::int16_t) == 2, "sizeof(tdsl::int16_t) is not 2 in your platform!");
static_assert(sizeof(tdsl::uint16_t) == 2, "sizeof(tdsl::uint16_t) is not 2 in your platform!");
static_assert(sizeof(tdsl::int32_t) == 4, "sizeof(tdsl::int32_t) is not 4 in your platform!");
static_assert(sizeof(tdsl::uint32_t) == 4, "sizeof(tdsl::uint32_t) is not 4 in your platform!");
static_assert(sizeof(tdsl::int64_t) == 8, "sizeof(tdsl::int64_t) is not 8 in your platform!");
static_assert(sizeof(tdsl::uint64_t) == 8, "sizeof(tdsl::uint64_t) is not 8 in your platform!");

// user-defined literals for casting integers

inline static constexpr tdsl::uint8_t operator"" _tdsu8(unsigned long long arg) noexcept {
    return static_cast<tdsl::uint8_t>(arg);
}

inline static constexpr tdsl::int8_t operator"" _tdsi8(unsigned long long arg) noexcept {
    return static_cast<tdsl::int8_t>(arg);
}

inline static constexpr tdsl::uint16_t operator"" _tdsu16(unsigned long long arg) noexcept {
    return static_cast<tdsl::uint16_t>(arg);
}

inline static constexpr tdsl::int16_t operator"" _tdsi16(unsigned long long arg) noexcept {
    return static_cast<tdsl::int16_t>(arg);
}

inline static constexpr tdsl::uint32_t operator"" _tdsu32(unsigned long long arg) noexcept {
    return static_cast<tdsl::uint32_t>(arg);
}

inline static constexpr tdsl::int32_t operator"" _tdsi32(unsigned long long arg) noexcept {
    return static_cast<tdsl::int32_t>(arg);
}

inline static constexpr tdsl::uint64_t operator"" _tdsu64(unsigned long long arg) noexcept {
    return static_cast<tdsl::uint64_t>(arg);
}

inline static constexpr tdsl::int64_t operator"" _tdsi64(unsigned long long arg) noexcept {
    return static_cast<tdsl::int64_t>(arg);
}

#endif