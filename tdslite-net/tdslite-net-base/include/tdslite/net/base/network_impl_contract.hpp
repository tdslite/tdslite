/**
 * _________________________________________________
 *
 * @file   abstract_net_interface.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   19.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_span.hpp>

namespace tdsl { namespace net {

    /**
     * Validator metaclass for ensuring that network
     * implementations has the required properties.
     *
     * @tparam ConcreteNetImpl
     * Concrete network implementation to validate
     */
    template <typename ConcreteNetImpl>
    struct network_impl_contract {

        template <typename T>
        using has_recv_member_fn_t = decltype(traits::declval<T>().do_recv(
            static_cast<tdsl::uint32_t>(0), typename T::read_at_least{}));

        template <typename T>
        using has_recv_member_fn = traits::is_detected<has_recv_member_fn_t, T>;

        template <typename T>
        using has_consume_recv_buf_fn_t = decltype(traits::declval<T>().do_consume_recv_buf(
            static_cast<tdsl::uint32_t>(0), static_cast<tdsl::uint32_t>(0)));

        template <typename T>
        using has_consume_recv_buf = traits::is_detected<has_consume_recv_buf_fn_t, T>;

        template <typename T>
        using has_consume_send_buf_fn_t = decltype(traits::declval<T>().do_consume_send_buf(
            static_cast<tdsl::uint32_t>(0), static_cast<tdsl::uint32_t>(0)));

        template <typename T>
        using has_consume_send_buf = traits::is_detected<has_consume_send_buf_fn_t, T>;

        template <typename T>
        using has_do_connect_fn_t = decltype(traits::declval<T>().do_connect(
            traits::declval<tdsl::char_view>(), static_cast<tdsl::uint16_t>(0)));

        template <typename T>
        using has_do_connect_fn = traits::is_detected<has_do_connect_fn_t, T>;

        template <typename T>
        using has_rbuf_reader_fn_t = decltype(traits::declval<T>().rbuf_reader());

        template <typename T>
        using has_rbuf_reader_fn = traits::is_detected<has_rbuf_reader_fn_t, T>;

        template <typename T>
        using has_sbuf_reader_fn_t = decltype(traits::declval<T>().sbuf_reader());

        template <typename T>
        using has_sbuf_reader_fn = traits::is_detected<has_sbuf_reader_fn_t, T>;

        /**
         * Validate
         */
        inline constexpr network_impl_contract() noexcept {
            // If you are hitting these static assertions, it means either your ConcreteNetImpl
            // does not have do_recv function, or it does not have the expected function signature.
            // static_assert(
            //     traits::dependent_bool<has_consume_recv_buf<ConcreteNetImpl>::value>::value,
            //     "The type ConcreteNetImpl must implement void "
            //     "do_consume_recv_buf(tdsl::uint32_t, tdsl::uint32_t) function!");

            // static_assert(
            //     traits::dependent_bool<has_consume_send_buf<ConcreteNetImpl>::value>::value,
            //     "The type ConcreteNetImpl must implement void "
            //     "do_consume_send_buf(tdsl::uint32_t, tdsl::uint32_t) function!");

            static_assert(traits::dependent_bool<has_recv_member_fn<ConcreteNetImpl>::value>::value,
                          "The type ConcreteNetImpl must implement void "
                          "do_recv(tdsl::uint32_t, read_at_least) function!");

            static_assert(traits::dependent_bool<has_do_connect_fn<ConcreteNetImpl>::value>::value,
                          "The type ConcreteNetImpl must implement void "
                          "do_connect(tdsl::char_view, "
                          "tdsl::uint16_t) function!");

            // static_assert(traits::dependent_bool<has_rbuf_reader_fn<ConcreteNetImpl>::value>::value,
            //               "The type ConcreteNetImpl must implement void rbuf_reader()
            //               function!");

            // static_assert(traits::dependent_bool<has_sbuf_reader_fn<ConcreteNetImpl>::value>::value,
            //               "The type ConcreteNetImpl must implement void sbuf_reader()
            //               function!");
        }
    };
}} // namespace tdsl::net