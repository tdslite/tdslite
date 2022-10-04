/**
 * _________________________________________________
 *
 * @file   net_impl_base.hpp
 * @author Mustafa Kemal GILOR <mgilor@nettsi.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/detail/tdsl_callback.hpp>
#include <tdslite/detail/tdsl_message_type.hpp>
#include <tdslite/detail/tdsl_message_token_type.hpp>
#include <tdslite/detail/tdsl_envchange_type.hpp>
#include <tdslite/detail/tdsl_packet_handler_result.hpp>
#include <tdslite/detail/tdsl_message_status.hpp>

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_debug_print.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_expected.hpp>

namespace tdsl { namespace net {

    namespace detail {

        template <typename T>
        using has_recv_member_fn_t = decltype(traits::declval<T>().do_recv(static_cast<tdsl::uint32_t>(0), typename T::read_at_least{}));

        template <typename T>
        using has_recv_member_fn = traits::is_detected<has_recv_member_fn_t, T>;

        template <typename T>
        using has_consume_recv_buf_fn_t =
            decltype(traits::declval<T>().do_consume_recv_buf(static_cast<tdsl::uint32_t>(0), static_cast<tdsl::uint32_t>(0)));

        template <typename T>
        using has_consume_recv_buf = traits::is_detected<has_consume_recv_buf_fn_t, T>;

    } // namespace detail

    template <typename ConcreteNetImpl>
    struct network_impl_base {
        struct read_exactly {};

        struct read_at_least {};

        struct netbuf_reader : public binary_reader<tdsl::endian::little> {
            netbuf_reader(ConcreteNetImpl & n, tdsl::span<const tdsl::uint8_t> buf) :
                binary_reader<tdsl::endian::little>{buf.data(), buf.size()}, nimplb(n) {

                static_assert(traits::dependent_bool<detail::has_consume_recv_buf<ConcreteNetImpl>::value>::value,
                              "The type ConcreteNetImpl must implement void do_consume_recv_buf(tdsl::uint32_t, tdsl::uint32_t) function!");
            }

            ~netbuf_reader() {
                nimplb.do_consume_recv_buf(offset(), 0);
            }

        private:
            ConcreteNetImpl & nimplb;
        }; // give rbuf_reader object
    private:
        // on destruct, trim read amount bytes from buf
        // rbuf_reader,

        inline void call_recv(tdsl::uint32_t min_amount, read_at_least) {
            static_cast<ConcreteNetImpl &>(*this).do_recv(min_amount, read_at_least{});
        }

        inline void call_recv(tdsl::uint32_t exact_amount, read_exactly) {
            static_cast<ConcreteNetImpl &>(*this).do_recv(exact_amount, read_exactly{});
        }

        inline expected<tdsl::uint32_t, tdsl::int32_t> call_read(tdsl::span<tdsl::uint8_t> dst_buf, read_exactly) {
            return static_cast<ConcreteNetImpl &>(*this).do_read(dst_buf, read_exactly{});
        }

        inline auto call_rbuf_reader() -> netbuf_reader {
            return static_cast<ConcreteNetImpl &>(*this).rbuf_reader();
        }

    public:
        using tds_msg_handler_result = packet_handler_result<bool, false>;

        /**
         * Packet data callback
         *
         * Returns amount of needed bytes, if any.
         */
        using tds_packet_data_callback =
            callback<void, tdsl::uint32_t (*)(void *, tdsl::detail::e_tds_message_type, tdsl::binary_reader<tdsl::endian::little> &)>;

        enum class e_conection_state : tdsl::int8_t
        {
            attempt_in_progress = 0,
            connected           = 1,
            disconnected        = -1,
            connection_failed   = -2,
            resolve_failed      = -3,
        };

        using connection_state_callback_ctx = callback<void, void (*)(/*user_ptr*/ void *, e_conection_state)>;

        network_impl_base() noexcept {
            // If you are hitting this static assertion, it means either your ConcreteNetImpl
            // does not have do_recv function, or it does not have the expected function signature.
            static_assert(traits::dependent_bool<detail::has_recv_member_fn<ConcreteNetImpl>::value>::value,
                          "The type ConcreteNetImpl must implement void do_recv(tdsl::uint32_t) function!");

            // static_assert(traits::dependent_bool<detail::has_recv_buf_member_fn<ConcreteNetImpl>::value>::value,
            //               "The type ConcreteNetImpl must implement tdsl::span<const tdsl::uint8_t> recv_buf() function!");
        }

        // begin_recv_tds_response

        /**
         * Receive one, complete TDS PDU.
         */
        void do_receive_tds_pdu() noexcept {

            // Each TDS message contains a status field
            // TDS payload can fragment between multiple messages
            // Each fragment will contain a TDS header
            // Last fragment will have its EOM bit set, others will not

            bool eom_flag                       = {false};
            constexpr static auto k_tds_hdr_len = 8;
            do {
                tdsl::uint8_t tds_hbuf [k_tds_hdr_len] = {0};
                tdsl::span<tdsl::uint8_t> tds_hbuf_s{tds_hbuf};
                call_read(tds_hbuf_s, read_exactly{});
                auto thdr_rdr = binary_reader<tdsl::endian::big>{tds_hbuf};

                // Type defines the type of message. Type is a 1-byte unsigned char.
                const tdsl::detail::e_tds_message_type message_type =
                    static_cast<tdsl::detail::e_tds_message_type>(thdr_rdr.read<tdsl::uint8_t>());
                // Status is a bit field used to indicate the message state. Status is a 1-byte
                // unsigned char.
                const auto status                  = thdr_rdr.read_raw<tdsl::detail::tds_message_status>();
                // Length is the size of the packet including the 8 bytes in the packet header.
                // It is the number of bytes from the start of this header to the start of the
                // next packet header. Length is a 2-byte, unsigned short and is represented
                // in network byte order (big-endian). The Length value MUST be greater than
                // or equal to 512 bytes and smaller than or equal to 32,767 bytes. The default
                // value is 4,096 bytes. Starting with TDS 7.3, the Length MUST be the negotiated
                // packet size when sending a packet from client to server, unless it is the last
                // packet of a request (that is, the EOM bit in Status is ON) or the client has not
                // logged in.
                constexpr static auto k_max_length = 32767;
                const auto length                  = thdr_rdr.read<tdsl::uint16_t>();
                if (length < k_tds_hdr_len || length > k_max_length) {
                    // invalid length
                    TDSL_ASSERT_MSG(0, "Invalid tds message length!");
                    return;
                }
                const auto packet_data_size = length - k_tds_hdr_len;
                // // Skip channel, packet id and window
                // nbuf_rdr.advance(/*amount_of_bytes=*/4);
                call_recv(packet_data_size, read_exactly{}); // may throw?

                // This is a netbuf_reader instance.
                // Read operations on this will be committed
                // to underlying buffer on object destruction
                // e.g. nmsg_rdr.read(2) will cause 2 bytes from
                // the start of the underlying buffer to be removed
                auto nmsg_rdr = call_rbuf_reader();
                // pass current buffer down
                if (not nmsg_rdr.has_bytes(packet_data_size)) {
                    // this should not happen
                    TDSL_ASSERT_MSG(0, "The receive buffer does not contain expected amount of bytes, something is wrong!");
                    return;
                }

                // TODO: check if downstream consumed the message
                // TODO: Check if we got enough space to pull the next message

                if (packet_data_cb) {
                    const auto needed_bytes = packet_data_cb.invoke(message_type, nmsg_rdr);
                    TDSL_DEBUG_PRINTLN("network_impl_base::handle_tds_response(...) -> msg_cb_ctx, need byte(s) value:%d", needed_bytes);
                    (void) needed_bytes;
                }
                // if (msg_cb_ctx.callback) {
                //     // pass reader down,
                //     auto need_bytes = msg_cb_ctx.callback(msg_cb_ctx.user_ptr, static_cast<msg_type>(mt), rr.read(rr.remaining_bytes()));
                //     TDSL_DEBUG_PRINT("network_impl_base::handle_tds_response(...) -> msg_cb_ctx, need byte(s) value:%d \n",
                //                      (length - rr.size_bytes()));
                //     return need_bytes;
                // }

                // pass current buffer down

                // if (not nbuf_rdr.has_bytes(length)) {
                //     call_recv(length);
                //     nbuf_rdr = call_rbuf_reader();
                // }

                eom_flag = status.end_of_message;
            } while (not eom_flag);

            // while(not eom_seen)<--------------------------|
            //  read(8)                                      |
            //  read_tds_header                              |
            //  read_current_fragment(as a whole)            |
            //  remove read bytes from buf                   |
            //  pass lower stack                             |
            //  remove consumed bytes from beginning         |
            //  shift remaining bytes to start of the buffer |
            //  repeat ---------------------------------------

            // ... or alternatively, find a way to make a circular
            // buffer contiguous.
        }

        /**
         * Set TDS packet data callback
         *
         * @param [in] user_ptr uptr
         * @param [in] cb New callback
         */
        TDSL_SYMBOL_VISIBLE void register_packet_data_callback(void * user_ptr, tds_packet_data_callback::function_type cb) {
            packet_data_cb.set(user_ptr, cb);
        }

    protected:
        tds_packet_data_callback packet_data_cb{};

    private:
        // void
    };
}} // namespace tdsl::net