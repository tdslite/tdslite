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
#include <tdslite/detail/tdsl_message_token_type.hpp>
#include <tdslite/detail/tdsl_envchange_type.hpp>
#include <tdslite/detail/tdsl_callback_ctx.hpp>
// #include <tdslite/net/base/msg_callback.hpp>

#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_byte_swap.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>

#define TDSLITE_TRY_READ_VARCHAR(TYPE, VARNAME, READER)                                                                                    \
    const auto VARNAME##_octets = (READER.read<TYPE>() * 2);                                                                               \
    if (not READER.has_bytes(VARNAME##_octets)) {                                                                                          \
        return VARNAME##_octets - READER.remaining_bytes();                                                                                \
    }                                                                                                                                      \
    const auto VARNAME = READER.read(VARNAME##_octets)

#define TDSLITE_TRY_READ_U16_VARCHAR(VARNAME, READER) TDSLITE_TRY_READ_VARCHAR(tdsl::uint16_t, VARNAME, READER)
#define TDSLITE_TRY_READ_U8_VARCHAR(VARNAME, READER)  TDSLITE_TRY_READ_VARCHAR(tdsl::uint8_t, VARNAME, READER)

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
                // static_assert(detail::has_set_receive_callback<Derived>::value,
                //               "The type NetImpl must implement void set_receive_callback(void*, tdsl::span<tdsl::uint8_t>)
                //               function!");
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

        using tds_context_type           = tds_context<NetImpl>;
        using xmit_context               = detail::net_packet_xmit_context<tds_context_type>;
        using recv_context               = detail::net_packet_recv_context<tds_context_type>;

        /**
         * The function type of the callback function that is going to be invoked
         * when data is received in a network implementation
         */
        using envchange_callback_fn_type = tdsl::uint32_t (*)(/*user_ptr*/ void *, /*msg_type*/ tdsl::detail::e_tds_envchange_type,
                                                              /*new_value*/ tdsl::span<const tdsl::uint8_t>,
                                                              /*old_value*/ tdsl::span<const tdsl::uint8_t>);

        // Environment change callback context
        using envchange_callback_ctx     = tdsl::callback_ctx<envchange_callback_fn_type>;

        tds_context() noexcept {
            this->register_msg_recv_callback(this, &handle_msg);
        }

        // using envchange_callback_fn_type =

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
            this->template write(/*arg=*/0x01_tdsu8);                   // STATUS
            this->template write(/*arg=*/0_tdsu16);                     // Placeholder for length
            this->template write(/*arg=*/0_tdsu32);                     // Channel, Packet ID and Window
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

        // /**
        //  * Set the receive callback object
        //  *
        //  * The given user_ptr will be supplied as the first argument to the callback function
        //  * on invocation.
        //  *
        //  * @param user_ptr User data pointer
        //  * @param cbfn The callback function
        //  */
        // inline void set_receive_callback(void * user_ptr, void (*cbfn)(void *, tdsl::span<const tdsl::uint8_t>)) noexcept {
        //     this->do_set_receive_callback(user_ptr, cbfn);
        // }

        /**
         * Default handler function for TDS messages.
         *
         * Dissects the incoming TDS message and invokes the corresponding callback function.
         *
         * @param [in] self_optr Opaque pointer to self (tds_context)
         * @param [in] mt Incoming message type
         * @param [in] message Incoming message data
         *
         * @return tdsl::uint32_t
         */
        static tdsl::uint32_t handle_msg(void * self_optr, tdsl::detail::e_tds_message_type mt, tdsl::span<const tdsl::uint8_t> message) {

            auto self        = reinterpret_cast<tds_context_type *>(self_optr);

            using msg_type   = tdsl::detail::e_tds_message_type;
            using token_type = tdsl::detail::e_tds_message_token_type;

            auto rr          = tdsl::binary_reader<tdsl::endian::little>{message};

            switch (mt) {
                case msg_type::tabular_result: {
                    while (rr.has_bytes(/*amount_of_bytes=*/3)) {
                        const auto tt = rr.read<tdsl::uint8_t>();
                        const auto ts = rr.read<tdsl::uint16_t>();

                        TDSLITE_ASSERT_MSG(ts < length,
                                           "Something is wrong, token length is greater than the length of the encapsulating TDS PDU!");

                        // Ensure that we got enough bytes to read the token
                        if (not(rr.has_bytes(ts))) {
                            return ts - rr.remaining_bytes();
                        }

                        auto token_reader            = rr.subreader(ts);

                        tdsl::uint32_t subhandler_nb = {0};

                        switch (static_cast<token_type>(tt)) {
                            case token_type::envchange: {
                                subhandler_nb = self->handle_envchange_token(token_reader);
                            } break;
                            case token_type::error: {
                                subhandler_nb = self->handle_error_token(token_reader);
                            } break;
                            case token_type::info: {
                                subhandler_nb = self->handle_info_token(token_reader);
                            } break;
                            case token_type::done: {
                                subhandler_nb = self->handle_done_token(token_reader);
                            } break;
                        }

                        if (subhandler_nb > 0) {
                            return subhandler_nb;
                        }

                        // Advance to the next token
                        rr.advance(ts);
                    }
                }
            }

            return 0;
        }

        /**
         * Size of the TDS header
         */
        inline static auto tds_header_size() noexcept -> tdsl::uint32_t {
            return sizeof(tds_header);
        }

        /**
         * Set environment info change callback
         *
         * @param [in] rcb New callback
         */
        TDSLITE_SYMBOL_VISIBLE void do_register_envchange_callback(void * user_ptr, envchange_callback_fn_type rcb) {
            envinfochg_cb_ctx.user_ptr = user_ptr;
            envinfochg_cb_ctx.callback = rcb;
        }

    private:
        inline tdsl::uint32_t handle_envchange_token(tdsl::binary_reader<tdsl::endian::little> & rr) noexcept {
            using envchange_type = tdsl::detail::e_tds_envchange_type;
            // Read ENVCHANGE type
            const auto ect       = static_cast<envchange_type>(rr.read<tdsl::uint8_t>());
            switch (ect) {

                // Handle envchange types in OLD = B_VARHAR, NEW = B_VARCHAR form
                case envchange_type::database:
                case envchange_type::language:
                case envchange_type::charset:
                case envchange_type::packet_size: {
                    // The reader must have at least 2 bytes (new value length, old value length)
                    if (not rr.has_bytes(/*amount_of_bytes=*/2)) {
                        return 2 - rr.remaining_bytes();
                    }

                    // Read new value
                    const auto nvlen_octets = (rr.read<tdsl::uint8_t>() * 2);
                    if (not rr.has_bytes(nvlen_octets)) {
                        return nvlen_octets - rr.remaining_bytes();
                    }
                    const auto nvval        = rr.read(nvlen_octets);

                    // Read old value
                    const auto ovlen_octets = (rr.read<tdsl::uint8_t>() * 2);
                    if (not rr.has_bytes(ovlen_octets)) {
                        return ovlen_octets - rr.remaining_bytes();
                    }
                    const auto ovval = rr.read(ovlen_octets);

                    // Call envchange callback here

                    envinfochg_cb_ctx.callback ? envinfochg_cb_ctx.callback(envinfochg_cb_ctx.user_ptr, ect, nvval, ovval)
                                               : static_cast<tdsl::uint32_t>(0);
                } break;
            }
            return 0;
        }

        inline tdsl::uint32_t handle_info_token(tdsl::binary_reader<tdsl::endian::little> & rr) {
            constexpr static auto k_min_info_bytes = 14;
            if (not rr.has_bytes(k_min_info_bytes)) {
                return k_min_info_bytes - rr.remaining_bytes();
            }

            // Number, State, Class, MsgText, ServerName, ProcName, LineNumber
            const auto number = rr.read<tdsl::uint32_t>();
            const auto state  = rr.read<tdsl::uint8_t>();
            const auto class_ = rr.read<tdsl::uint8_t>();

            TDSLITE_TRY_READ_U16_VARCHAR(msgtext, rr);
            TDSLITE_TRY_READ_U8_VARCHAR(server_name, rr);
            TDSLITE_TRY_READ_U8_VARCHAR(proc_name, rr);

            const auto line_number = rr.read<tdsl::uint16_t>();

            (void) number, (void) state, (void) class_, (void) msgtext, (void) server_name, (void) proc_name, (void) line_number;
            TDSLITE_ASSERT_MSG(rr.remaining_bytes() == 0, "There are unhandled stray bytes in INFO token, probably something is wrong!");
            return 0;
        }

        inline tdsl::uint32_t handle_error_token(tdsl::binary_reader<tdsl::endian::little> & rr) {
            (void) rr;
            return 0;
        }

        inline tdsl::uint32_t handle_done_token(tdsl::binary_reader<tdsl::endian::little> & rr) {
            (void) rr;
            return 0;
        }
        // Environment info change callback context
        envchange_callback_ctx envinfochg_cb_ctx{};
        friend struct detail::net_packet_xmit_context<tds_context<NetImpl>>;
        friend struct detail::net_packet_recv_context<tds_context<NetImpl>>;
    };
}} // namespace tdsl::detail