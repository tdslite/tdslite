/**
 * _________________________________________________
 *
 * @file   tdsl_tds_context.hpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/detail/tdsl_message_type.hpp>

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_byte_swap.hpp>

namespace tdsl { namespace detail {

    namespace detail {

        /**
         * Network packet transmit context
         *
         * @tparam Derived Derived type (CRTP)
         */
        template <typename Derived>
        struct net_packet_xmit_context {

            //
            template <typename T, traits::enable_if_integral<T> = true>
            inline auto write(T v) noexcept -> void {
                tdsl::span<const tdsl::uint8_t> data(reinterpret_cast<const tdsl::uint8_t *>(&v), sizeof(T));
                write(data);
            }

            template <typename T, traits::enable_if_integral<T> = true>
            inline auto write_be(T v) noexcept -> void {
                write(native_to_be(v));
            }

            template <typename T, traits::enable_if_integral<T> = true>
            inline auto write_le(T v) noexcept -> void {
                write(native_to_le(v));
            }

            template <typename T, traits::enable_if_integral<T> = true>
            inline auto write(tdsl::uint32_t offset, T v) noexcept -> void {
                tdsl::span<const tdsl::uint8_t> data(reinterpret_cast<const tdsl::uint8_t *>(&v), sizeof(T));
                write(offset, data);
            }

            template <typename T, traits::enable_if_integral<T> = true>
            inline auto write_be(tdsl::uint32_t offset, T v) noexcept -> void {
                write(offset, native_to_be(v));
            }

            template <typename T, traits::enable_if_integral<T> = true>
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
                static_cast<Derived &>(*this).do_send(TDSLITE_FORWARD(args)...);
            }
        };

        // --------------------------------------------------------------------------------

        /**
         * Network packet receive context
         *
         * @tparam Derived Derived type (CRTP)
         */
        template <typename Derived>
        struct net_packet_recv_context {
            template <typename... Args>
            inline void recv(Args &&... args) {
                static_cast<Derived &>(*this)->do_recv(TDSLITE_FORWARD(args)...);
            }
        };

        // --------------------------------------------------------------------------------

        template <typename T>
        using set_receive_callback_member_fn_t = decltype(traits::declval<T>().do_set_receive_callback(
            static_cast<void *>(0), static_cast<void (*)(void *, tdsl::span<const tdsl::uint8_t>)>(0)));

        template <typename T>
        using has_set_receive_callback = traits::is_detected<set_receive_callback_member_fn_t, T>;

        /**
         * Poor man's `concept` for checking whether the given Derived type implements
         * the required functions for a proper network implementation.
         *
         * @tparam Derived The type to check
         */
        template <typename Derived>
        struct is_network_interface_implemented {
            constexpr is_network_interface_implemented() {
                static_assert(detail::has_set_receive_callback<Derived>::value,
                              "The type NetImpl must implement void set_receive_callback(void*, tdsl::span<tdsl::uint8_t>) function!");
            }
        };

    } // namespace detail

    /**
     * Base type for all TDS message contexts
     *
     * @tparam NetImpl Network-layer Implementation
     * @tparam TYPE Message type
     */
    template <typename NetImpl>
    struct tds_context : public NetImpl,
                         public detail::net_packet_xmit_context<tds_context<NetImpl>>,
                         public detail::net_packet_recv_context<tds_context<NetImpl>>,
                         private detail::is_network_interface_implemented<tds_context<NetImpl>> {

        using tds_context_type = tds_context<NetImpl>;
        using xmit_context     = detail::net_packet_xmit_context<tds_context_type>;
        using recv_context     = detail::net_packet_recv_context<tds_context_type>;

        /**
         * The tabular data stream protocol header
         */
        struct tds_header {
            tdsl::uint8_t type;
            tdsl::uint8_t status;
            tdsl::uint16_t length;
            tdsl::uint16_t channel;
            tdsl::uint8_t packet_number;
            tdsl::uint8_t window;
        } TDSLITE_PACKED;

        /**
         * Write common TDS header of the packet
         *
         * @note 16-bit zero value will be put for the `length` field as a placeholder.
         * The real packet length must be substituted via calling @ref put_tds_header_length
         * function afterwards.
         */
        inline void write_tds_header(e_tds_message_type msg_type) noexcept {
            this->template write(static_cast<tdsl::uint8_t>(msg_type)); // Packet type
            this->template write(0x01_tdsu8);                           // STATUS
            this->template write(0_tdsu16);                             // Placeholder for length
            this->template write(0_tdsu32);                             // Channel, Packet ID and Window
        }

        /**
         * Put the packet length into TDS packet.
         *
         * @note @ref write_tds_header() function must already be called
         * before
         *
         * @param [in] data_length Length of the data section
         */
        void put_tds_header_length(tdsl::uint16_t data_length) noexcept {
            // Length is the size of the packet inclusing the 8 bytes in the packet header.
            // It is the number of bytes from start of this header to the start of the next packet header.
            // Length is a 2-byte, unsigned short and is represented in network byte order (big-endian).
            this->template write(TDSLITE_OFFSETOF(tds_header, length), host_to_network(static_cast<tdsl::uint16_t>(data_length + 8)));
        }

        /**
         * Set the receive callback object
         *
         * The given user_ptr will be supplied as the first argument to the callback function
         * on invocation.
         *
         * @param user_ptr User data pointer
         * @param cbfn The callback function
         */
        inline void set_receive_callback(void * user_ptr, void (*cbfn)(void *, tdsl::span<const tdsl::uint8_t>)) noexcept {
            this->do_set_receive_callback(user_ptr, cbfn);
        }

        /**
         * Size of the TDS header
         */
        inline static auto tds_header_size() noexcept -> tdsl::uint32_t {
            return sizeof(tds_header);
        }

    private:
        friend struct detail::net_packet_xmit_context<tds_context<NetImpl>>;
        friend struct detail::net_packet_recv_context<tds_context<NetImpl>>;
    };
}} // namespace tdsl::detail