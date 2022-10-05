/**
 * _________________________________________________
 *
 * @file   tdsl_net_send_if_mixin.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   04.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_byte_swap.hpp>

namespace tdsl { namespace detail {
    // --------------------------------------------------------------------------------

    /**
     * Network packet transmit context
     *
     * @tparam Derived Derived type (CRTP)
     */
    template <typename Derived>
    struct net_send_if_mixin {

        template <typename T, traits::enable_when::integral<T> = true>
        inline auto write(T v) noexcept -> void {
            tdsl::span<const tdsl::uint8_t> data(reinterpret_cast<const tdsl::uint8_t *>(&v), sizeof(T));
            write(data);
        }

        template <typename T, traits::enable_when::integral<T> = true>
        inline auto write_be(T v) noexcept -> void {
            write(native_to_be(v));
        }

        template <typename T, traits::enable_when::integral<T> = true>
        inline auto write_le(T v) noexcept -> void {
            write(native_to_le(v));
        }

        template <typename T, traits::enable_when::integral<T> = true>
        inline auto write(tdsl::uint32_t offset, T v) noexcept -> void {
            tdsl::span<const tdsl::uint8_t> data(reinterpret_cast<const tdsl::uint8_t *>(&v), sizeof(T));
            write(offset, data);
        }

        template <typename T, traits::enable_when::integral<T> = true>
        inline auto write_be(tdsl::uint32_t offset, T v) noexcept -> void {
            write(offset, native_to_be(v));
        }

        template <typename T, traits::enable_when::integral<T> = true>
        inline auto write_le(tdsl::uint32_t offset, T v) noexcept -> void {
            write(offset, native_to_le(v));
        }

        template <typename T>
        inline void write(tdsl::uint32_t offset, tdsl::span<T> data) noexcept {
            static_cast<Derived &>(*this).do_write(offset, data);
        }

        template <typename T>
        inline void write(tdsl::span<T> data) noexcept {
            static_cast<Derived &>(*this).do_write(data);
        }

        template <typename... Args>
        inline void send(Args &&... args) noexcept {
            static_cast<Derived &>(*this).do_send(TDSL_FORWARD(args)...);
        }
    };

}} // namespace tdsl::detail