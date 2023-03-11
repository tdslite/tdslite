/**
 * ____________________________________________________
 *
 * @file   tdsl_net_rx_mixin.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   04.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_NET_RECV_IF_MIXIN_HPP
#define TDSL_DETAIL_NET_RECV_IF_MIXIN_HPP

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/detail/tdsl_message_type.hpp>
#include <tdslite/detail/tdsl_message_type.hpp>
#include <tdslite/util/tdsl_endian.hpp>

namespace tdsl { namespace detail {
    // --------------------------------------------------------------------------------

    template <typename T>
    using register_packet_data_callback_member_fn_t =
        decltype(traits::declval<T>().register_packet_data_callback(
            static_cast<tdsl::uint32_t (*)(void *, tdsl::detail::e_tds_message_type,
                                           tdsl::binary_reader<tdsl::endian::little> &)>(0),
            static_cast<void *>(0)));

    template <typename T>
    using has_register_packet_data_callback_member_fn =
        traits::is_detected<register_packet_data_callback_member_fn_t, T>;

    // --------------------------------------------------------------------------------

    template <typename T>
    using do_receive_tds_pdu_member_fn_t = decltype(traits::declval<T>().do_receive_tds_pdu());

    template <typename T>
    using has_do_receive_tds_pdu_member_fn = traits::is_detected<do_receive_tds_pdu_member_fn_t, T>;

    // --------------------------------------------------------------------------------

    /**
     * Network implementation receive interface
     *
     * Derive from this class
     *
     * @tparam Derived Derived type (CRTP)
     */
    template <typename Derived>
    struct net_rx_mixin {
        net_rx_mixin() {
            // If you are hitting these static assertions, it means either your NetImpl does not
            // have the following functions, or the functions does not have the expected function
            // signature. These functions are provided by the `network_impl_base` type, which MUST
            // be inherited by every single NetImpl. So, either the NetImpl you have provided does
            // not inherit from `network_impl_base`, or the inheritance is private. Go figure.
            static_assert(
                traits::dependent_bool<
                    detail::has_register_packet_data_callback_member_fn<Derived>::value>::value,
                "The type NetImpl must implement void register_msg_recv_callback("
                "tdsl::uint32_t (*)(void *, "
                "tdsl::detail::e_tds_message_type, byte_view), void*) function!");
            static_assert(traits::dependent_bool<
                              detail::has_do_receive_tds_pdu_member_fn<Derived>::value>::value,
                          "The type NetImpl must implement void do_receive_tds_pdu() function!");
        } // namespace detail

        // --------------------------------------------------------------------------------

        /**
         * Receive one, complete TDS message.
         */
        inline void receive_tds_pdu() noexcept {
            static_cast<Derived &>(*this).do_receive_tds_pdu();
        }
    }; // namespace tdsl
}}     // namespace tdsl::detail

#endif