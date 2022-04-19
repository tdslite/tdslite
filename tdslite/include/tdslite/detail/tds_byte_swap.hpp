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

#include <tdslite/detail/tds_inttypes.hpp>
#include <tdslite/detail/tds_endian.hpp>
#include <tdslite/detail/tds_type_traits.hpp>

namespace tdslite { namespace detail {

    template <typename T>
    inline constexpr auto byte_swap(T v) noexcept -> T;

    template <>
    inline constexpr auto byte_swap<tdslite::int8_t>(tdslite::int8_t v) noexcept -> tdslite::int8_t {
        return v;
    }

    template <>
    inline constexpr auto byte_swap<tdslite::uint8_t>(tdslite::uint8_t v) noexcept -> tdslite::uint8_t {
        return v;
    }

    template <>
    inline constexpr auto byte_swap<tdslite::int16_t>(tdslite::int16_t v) noexcept -> tdslite::int16_t {
        return static_cast<tdslite::int16_t>(__builtin_bswap16(v));
    }

    template <>
    inline constexpr auto byte_swap<tdslite::uint16_t>(tdslite::uint16_t v) noexcept -> tdslite::uint16_t {
        return __builtin_bswap16(v);
    }

    template <>
    inline constexpr auto byte_swap<tdslite::int32_t>(tdslite::int32_t v) noexcept -> tdslite::int32_t {
        return static_cast<tdslite::int32_t>(__builtin_bswap32(v));
    }

    template <>
    inline constexpr auto byte_swap<tdslite::uint32_t>(tdslite::uint32_t v) noexcept -> tdslite::uint32_t {
        return __builtin_bswap32(v);
    }

    template <>
    inline constexpr auto byte_swap<tdslite::int64_t>(tdslite::int64_t v) noexcept -> tdslite::int64_t {
        return static_cast<tdslite::int64_t>(__builtin_bswap64(v));
    }

    template <>
    inline constexpr auto byte_swap<tdslite::uint64_t>(tdslite::uint64_t v) noexcept -> tdslite::uint64_t {
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
    template <tdslite::endian FromEndianness = tdslite::endian::native, tdslite::endian ToEndianness = tdslite::endian::non_native,
              typename T>
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
    template <tdslite::endian FromEndianness = tdslite::endian::native, tdslite::endian ToEndianness = tdslite::endian::non_native,
              typename T>
    inline constexpr auto swap_endianness(T v) noexcept -> typename traits::enable_if<!(FromEndianness == ToEndianness), T>::type {
        return byte_swap<decltype(v)>(v);
    }

    template <typename T>
    inline constexpr auto native_to_le(T v) noexcept -> T {
        return swap_endianness<tdslite::endian::native, tdslite::endian::little, T>(v);
    }

    template <typename T>
    inline constexpr auto le_to_native(T v) noexcept -> T {
        return swap_endianness<tdslite::endian::little, tdslite::endian::native, T>(v);
    }

    template <typename T>
    inline constexpr auto native_to_be(T v) noexcept -> T {
        return swap_endianness<tdslite::endian::native, tdslite::endian::big, T>(v);
    }

    template <typename T>
    inline constexpr auto be_to_native(T v) noexcept -> T {
        return swap_endianness<tdslite::endian::big, tdslite::endian::native, T>(v);
    }

    template <typename T>
    inline constexpr auto network_to_host(T v) noexcept -> T {
        return be_to_native(v);
    }

    template <typename T>
    inline constexpr auto host_to_network(T v) noexcept -> T {
        return native_to_be(v);
    }
}} // namespace tdslite::detail

#endif
