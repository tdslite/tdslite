/**
 * _________________________________________________
 * Compile-time checker for network I/O type constraints
 *
 * @file   network_io_contract.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   19.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSL_NET_NETWORK_IO_CONTRACT_HPP
#define TDSL_NET_NETWORK_IO_CONTRACT_HPP

#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_span.hpp>

namespace tdsl { namespace net {

    /**
     * Validator metaclass for ensuring that network
     * implementations has the required functions and
     * properties.
     *
     * A minimal network implementation is required to
     * implement the following functions:
     *
     *    tdsl::int32_t do_connect(tdsl::char_view target, tdsl::uint16_t port);
     *    tdsl::int32_t do_disconnect() noexcept;
     *    tdsl::int32_t do_send(byte_view header, byte_view message) noexcept;
     *    expected<tdsl::uint32_t, tdsl::int32_t> do_recv(tdsl::uint32_t transfer_amount) noexcept;
     *    expected<tdsl::uint32_t, tdsl::int32_t> do_recv(tdsl::uint32_t exact_amount,
     *                                                    byte_span dst_buf);
     *
     * @tparam Implementation Concrete network implementation to validate
     */
    template <typename Implementation>
    struct network_io_contract {

        template <typename T>
        using has_recv_member_fn_1_t =
            decltype(traits::declval<T>().do_recv(static_cast<tdsl::uint32_t>(0)));

        template <typename T>
        using has_recv_member_fn_1 = traits::is_detected<has_recv_member_fn_1_t, T>;

        template <typename T>
        using has_recv_member_fn_2_t = decltype(traits::declval<T>().do_recv(
            static_cast<tdsl::uint32_t>(0), tdsl::byte_span{}));

        template <typename T>
        using has_recv_member_fn_2 = traits::is_detected<has_recv_member_fn_2_t, T>;

        template <typename T>
        using has_send_member_fn_t =
            decltype(traits::declval<T>().do_send(tdsl::byte_view{}, tdsl::byte_view{}));

        template <typename T>
        using has_send_member_fn = traits::is_detected<has_send_member_fn_t, T>;

        template <typename T>
        using has_do_connect_fn_t = decltype(traits::declval<T>().do_connect(
            traits::declval<tdsl::char_view>(), static_cast<tdsl::uint16_t>(0)));

        template <typename T>
        using has_do_connect_fn = traits::is_detected<has_do_connect_fn_t, T>;

        template <typename T>
        using has_do_disconnect_fn_t = decltype(traits::declval<T>().do_disconnect());

        template <typename T>
        using has_do_disconnect_fn = traits::is_detected<has_do_disconnect_fn_t, T>;

        /**
         * Validate
         */
        inline constexpr network_io_contract() noexcept {
            // If you are hitting these static assertions, it means either your ConcreteNetImpl
            // does not have do_recv function, or it does not have the expected function
            // signature.

            static_assert(
                traits::dependent_bool<has_recv_member_fn_1<Implementation>::value>::value,
                "The type Implementation must implement `expected<tdsl::uint32_t, tdsl::int32_t> "
                "do_recv(tdsl::uint32_t)` function!");

            static_assert(
                traits::dependent_bool<has_recv_member_fn_2<Implementation>::value>::value,
                "The type Implementation must implement `expected<tdsl::uint32_t, tdsl::int32_t> "
                "do_recv(tdsl::uint32_t, tdsl::byte_span)` function!");

            static_assert(traits::dependent_bool<has_send_member_fn<Implementation>::value>::value,
                          "The type Implementation must implement `tdsl::int32_t "
                          "do_send(tdsl::byte_view, tdsl::byte_view)` function!");

            static_assert(traits::dependent_bool<has_do_connect_fn<Implementation>::value>::value,
                          "The type ConcreteNetImpl must implement `tdsl::int32_t "
                          "do_connect(tdsl::char_view, "
                          "tdsl::uint16_t)` function!");

            static_assert(traits::dependent_bool<has_do_connect_fn<Implementation>::value>::value,
                          "The type ConcreteNetImpl must implement `tdsl::int32_t "
                          "do_disconnect()` function!");
        }
    };
}} // namespace tdsl::net

#endif