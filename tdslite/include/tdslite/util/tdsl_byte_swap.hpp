/**
 * ____________________________________________________
 * Utility functions for swapping byte order of
 * integers for different endianness.
 *
 * @file   tds_byte_swap.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSLITE_DETAIL_TDS_BYTE_SWAP_HPP
#define TDSLITE_DETAIL_TDS_BYTE_SWAP_HPP

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_endian.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>

namespace tdsl {

    template <typename T, typename = traits::is_integral<T>>
    inline constexpr auto byte_swap(T v) noexcept -> T;

    template <>
    inline constexpr auto byte_swap<tdsl::int8_t>(tdsl::int8_t v) noexcept -> tdsl::int8_t {
        return v;
    }

    template <>
    inline constexpr auto byte_swap<tdsl::uint8_t>(tdsl::uint8_t v) noexcept -> tdsl::uint8_t {
        return v;
    }

    template <>
    inline constexpr auto byte_swap<tdsl::int16_t>(tdsl::int16_t v) noexcept -> tdsl::int16_t {
        return static_cast<tdsl::int16_t>(__builtin_bswap16(v));
    }

    template <>
    inline constexpr auto byte_swap<tdsl::uint16_t>(tdsl::uint16_t v) noexcept -> tdsl::uint16_t {
        return __builtin_bswap16(v);
    }

    template <>
    inline constexpr auto byte_swap<tdsl::int32_t>(tdsl::int32_t v) noexcept -> tdsl::int32_t {
        return static_cast<tdsl::int32_t>(__builtin_bswap32(v));
    }

    template <>
    inline constexpr auto byte_swap<tdsl::uint32_t>(tdsl::uint32_t v) noexcept -> tdsl::uint32_t {
        return __builtin_bswap32(v);
    }

    template <>
    inline constexpr auto byte_swap<tdsl::int64_t>(tdsl::int64_t v) noexcept -> tdsl::int64_t {
        return static_cast<tdsl::int64_t>(__builtin_bswap64(v));
    }

    template <>
    inline constexpr auto byte_swap<tdsl::uint64_t>(tdsl::uint64_t v) noexcept -> tdsl::uint64_t {
        return __builtin_bswap64(v);
    }

    /**
     * Swap endianness of the integer value @p v if FromEndianness is different from ToEndianness
     *
     * @tparam FromEndianness Endianness of @p v
     * @tparam ToEndianness The desired endianness for @p v
     * @tparam T Integer type
     * @param v Value
     * @returns Endianness-swapped version of @p v if FE != TE
     * @returns @p v if FE == TE
     */
    template <tdsl::endian FromEndianness = tdsl::endian::native, tdsl::endian ToEndianness = tdsl::endian::non_native, typename T>
    inline constexpr auto swap_endianness(T v) noexcept -> typename traits::enable_if<FromEndianness == ToEndianness, T>::type {
        return v;
    }

    /**
     * Swap endianness of the integer value @p v if FromEndianness is different from ToEndianness
     *
     * @tparam FromEndianness Endianness of @p v
     * @tparam ToEndianness The desired endianness for @p v
     * @tparam T Integer type
     * @param v Value
     * @returns Endianness-swapped version of @p v if FE != TE
     * @returns @p v if FE == TE
     */
    template <tdsl::endian FromEndianness = tdsl::endian::native, tdsl::endian ToEndianness = tdsl::endian::non_native, typename T>
    inline constexpr auto swap_endianness(T v) noexcept -> typename traits::enable_if<!(FromEndianness == ToEndianness), T>::type {
        return byte_swap<decltype(v)>(v);
    }

    template <typename T>
    inline constexpr auto native_to_le(T v) noexcept -> T {
        return swap_endianness<tdsl::endian::native, tdsl::endian::little, T>(v);
    }

    template <typename T>
    inline constexpr auto le_to_native(T v) noexcept -> T {
        return swap_endianness<tdsl::endian::little, tdsl::endian::native, T>(v);
    }

    template <typename T>
    inline constexpr auto native_to_be(T v) noexcept -> T {
        return swap_endianness<tdsl::endian::native, tdsl::endian::big, T>(v);
    }

    template <typename T>
    inline constexpr auto be_to_native(T v) noexcept -> T {
        return swap_endianness<tdsl::endian::big, tdsl::endian::native, T>(v);
    }

    template <typename T>
    inline constexpr auto network_to_host(T v) noexcept -> T {
        return be_to_native(v);
    }

    template <typename T>
    inline constexpr auto host_to_network(T v) noexcept -> T {
        return native_to_be(v);
    }
} // namespace tdsl

#endif
