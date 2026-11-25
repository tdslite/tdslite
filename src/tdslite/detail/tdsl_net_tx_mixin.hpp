/**
 * ____________________________________________________
 *
 * @file   tdsl_net_tx_mixin.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   04.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_NET_SEND_IF_MIXIN_HPP
#define TDSL_DETAIL_NET_SEND_IF_MIXIN_HPP

#include <tdslite/detail/tdsl_message_type.hpp>
#include <tdslite/detail/tdsl_send_status.hpp>

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_byte_swap.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>

namespace tdsl { namespace detail {

    // --------------------------------------------------------------------------------

    /**
     * Network packet transmit context
     *
     * @tparam Derived Derived type (CRTP)
     */
    template <typename Derived>
    struct net_tx_mixin {

        // --------------------------------------------------------------------------------

        template <typename T, traits::enable_when::integral<T> = true>
        inline auto write(T v) noexcept -> void {
            byte_view data(reinterpret_cast<const tdsl::uint8_t *>(&v), sizeof(T));
            write(data);
        }

        // --------------------------------------------------------------------------------

        template <typename T, traits::enable_when::integral<T> = true>
        inline auto write_be(T v) noexcept -> void {
            write(native_to_be(v));
        }

        // --------------------------------------------------------------------------------

        template <typename T, traits::enable_when::integral<T> = true>
        inline auto write_le(T v) noexcept -> void {
            write(native_to_le(v));
        }

        // --------------------------------------------------------------------------------

        template <typename T, traits::enable_when::integral<T> = true>
        inline auto write(tdsl::size_t offset, T v) noexcept -> void {
            byte_view data(reinterpret_cast<const tdsl::uint8_t *>(&v), sizeof(T));
            write(offset, data);
        }

        // --------------------------------------------------------------------------------

        template <typename T, traits::enable_when::integral<T> = true>
        inline auto write_be(tdsl::size_t offset, T v) noexcept -> void {
            write(offset, native_to_be(v));
        }

        // --------------------------------------------------------------------------------

        template <typename T, traits::enable_when::integral<T> = true>
        inline auto write_le(tdsl::size_t offset, T v) noexcept -> void {
            write(offset, native_to_le(v));
        }

        // --------------------------------------------------------------------------------

        template <typename T>
        inline void write(tdsl::size_t offset, tdsl::span<T> data) noexcept {
            static_cast<Derived &>(*this).do_write(offset, data);
        }

        // --------------------------------------------------------------------------------

        template <typename T>
        inline void write(tdsl::span<T> data) noexcept {
            static_cast<Derived &>(*this).do_write(data);
        }

        // --------------------------------------------------------------------------------

        template <typename... Args>
        inline void send(Args &&... args) noexcept {
            static_cast<Derived &>(*this).do_send(TDSL_FORWARD(args)...);
        }

        // --------------------------------------------------------------------------------

        /**
         * Send the message built in the network buffer
         *
         * @return e_send_status::success if sent
         * @return e_send_status::request_too_large if the message did not fit
         *         into the network buffer and was discarded
         * @return e_send_status::network_error if sending failed
         */
        TDSL_NODISCARD inline e_send_status
        send_tds_pdu(detail::e_tds_message_type mtype) noexcept {
            return static_cast<Derived &>(*this).do_send_tds_pdu(mtype);
        }

        // --------------------------------------------------------------------------------

        template <typename T>
        struct placeholder {

            inline placeholder(net_tx_mixin<Derived> & tx, tdsl::size_t offset) :
                tx(tx), offset(offset) {}

            // --------------------------------------------------------------------------------

            inline void write_be(T v) noexcept {
                tx.write_be(offset, v);
            }

            // --------------------------------------------------------------------------------

            inline void write_le(T v) noexcept {
                tx.write_le(offset, v);
            }

        private:
            net_tx_mixin<Derived> & tx;
            tdsl::size_t offset;
        };

        // --------------------------------------------------------------------------------

        /**
         * Put a placeholder value to current offset in order
         * to be filled later. Use returned placeholder object
         * to substitute the real value.
         *
         * @tparam T Placeholder value type
         *
         * @param [in] v Placeholder value. The actual value is ,
         *               not important, but the type is.
         *
         * @return placeholder<T> Placeholder object
         */
        template <typename T, traits::enable_when::integral<T> = true>
        inline placeholder<T> put_placeholder(T v) noexcept {
            auto beg = static_cast<Derived &>(*this).do_get_write_offset();
            write(v);
            return placeholder<T>{*this, beg};
        }
    };

}} // namespace tdsl::detail

#endif