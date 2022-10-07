/**
 * _________________________________________________
 *
 * @file   net_impl_base.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
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
        using has_do_connect_fn_t = decltype(traits::declval<T>().do_connect(
            traits::declval<tdsl::span<const char>>(), static_cast<tdsl::uint16_t>(0)));

        template <typename T>
        using has_do_connect_fn = traits::is_detected<has_do_connect_fn_t, T>;

        // int asio_network_impl::do_connect(tdsl::span<const char> target, tdsl::uint16_t port) {

    } // namespace detail

    template <typename ConcreteNetImpl>
    struct network_impl_base {
        struct read_exactly {};

        struct read_at_least {};

        struct netbuf_reader : public binary_reader<tdsl::endian::little> {
            netbuf_reader(ConcreteNetImpl & n, tdsl::span<const tdsl::uint8_t> buf) :
                binary_reader<tdsl::endian::little>{buf.data(), buf.size()}, nimplb(n) {

                static_assert(traits::dependent_bool<
                                  detail::has_consume_recv_buf<ConcreteNetImpl>::value>::value,
                              "The type ConcreteNetImpl must implement void "
                              "do_consume_recv_buf(tdsl::uint32_t, tdsl::uint32_t) function!");
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

        inline expected<tdsl::uint32_t, tdsl::int32_t> call_read(tdsl::span<tdsl::uint8_t> dst_buf,
                                                                 read_exactly) {
            return static_cast<ConcreteNetImpl &>(*this).do_read(dst_buf, read_exactly{});
        }

        inline auto call_rbuf_reader() -> netbuf_reader {
            return static_cast<ConcreteNetImpl &>(*this).rbuf_reader();
        }

        // sbuf_reader?

    public:
        using tds_msg_handler_result = packet_handler_result<bool, false>;

        /**
         * Packet data callback
         *
         * Returns amount of needed bytes, if any.
         */
        using tds_packet_data_callback =
            callback<void, tdsl::uint32_t (*)(void *, tdsl::detail::e_tds_message_type,
                                              tdsl::binary_reader<tdsl::endian::little> &)>;

        enum class e_conection_state : tdsl::int8_t
        {
            attempt_in_progress = 0,
            connected           = 1,
            disconnected        = -1,
            connection_failed   = -2,
            resolve_failed      = -3,
        };

        using connection_state_callback_ctx =
            callback<void, void (*)(/*user_ptr*/ void *, e_conection_state)>;

        network_impl_base() noexcept {
            // If you are hitting this static assertion, it means either your ConcreteNetImpl
            // does not have do_recv function, or it does not have the expected function signature.
            static_assert(
                traits::dependent_bool<detail::has_recv_member_fn<ConcreteNetImpl>::value>::value,
                "The type ConcreteNetImpl must implement void do_recv(tdsl::uint32_t) function!");

            static_assert(
                traits::dependent_bool<detail::has_do_connect_fn<ConcreteNetImpl>::value>::value,
                "The type ConcreteNetImpl must implement void do_connect(tdsl::span<const char>, "
                "tdsl::uint16_t) function!");

            // static_assert(traits::dependent_bool<detail::has_recv_buf_member_fn<ConcreteNetImpl>::value>::value,
            //               "The type ConcreteNetImpl must implement tdsl::span<const
            //               tdsl::uint8_t> recv_buf() function!");
        }

        /**
         * Connect to @p host: @p port
         *
         * @param [in] host Hostname or IP address
         * @param [in] port TCP port number
         * @return int
         */
        TDSL_NODISCARD inline int connect(tdsl::span<const char> host,
                                          tdsl::uint16_t port) noexcept {
            return static_cast<ConcreteNetImpl &>(*this).do_connect(host, port);
        }

        /**
         * Receive one, complete TDS PDU.
         */
        tdsl::uint32_t do_receive_tds_pdu() noexcept {

            /**
             * TDS packet data can span multiple TDS messages.
             * In such scenarios, each TDS message will have its
             * EOM flag set to false except the last TDS message
             * for the packet data.
             */
            bool eom_flag                              = {false};

            /**
             * The amount of successfully read TDS messages.
             * (for diagnostic purposes only)
             */
            tdsl::uint32_t processed_tds_message_count = 0;
            constexpr static auto k_tds_hdr_len        = 8;

            /**
             * The main TDS message receive loop.
             *
             * The loop is mildly stream-based, meaning it will not
             * wait for a whole TDS packet data to arrive before
             * delivering the data down to parsing, but it waits until
             * we have a complete TDS message before doing so.
             *
             * The downstream parsing code immediately parses the
             * tokens and fires the callback functions, which allows
             * us to discard the processed data from receive buffer
             * (e.g. a processed ROW token's data)
             */
            do {
                // We're using a stack buffer to read TDS header
                // because we only need it to determine the amount of
                // bytes to expect in the message and whether the received
                // message is the final one. The downstream parser does
                // not need these information. Therefore, we only read
                // packet data into internal packet buffer of network
                // stack, which allows us to handle fragmented packets
                // more easily (no header data to remove from buffer)
                tdsl::uint8_t tds_hbuf [k_tds_hdr_len] = {0};
                tdsl::span<tdsl::uint8_t> tds_hbuf_s{tds_hbuf};

                // We demand exactly `k_tds_hdr_len` bytes,even if the network
                // receive buffer has more available. This is done preventing
                // fragmented reads. When we read the header first, we know
                // exactly how many bytes we will need to read the whole message
                // so the second read call becomes a `read_exactly` call too,
                // which allows us to avoid dealing with fragmentation at TCP
                // level.
                call_read(tds_hbuf_s, read_exactly{});
                auto thdr_rdr = binary_reader<tdsl::endian::big>{tds_hbuf};

                // Type defines the type of message. Type is a 1-byte unsigned char.
                const tdsl::detail::e_tds_message_type message_type =
                    static_cast<tdsl::detail::e_tds_message_type>(thdr_rdr.read<tdsl::uint8_t>());

                // Status is a bit field used to indicate the message state. Status is a 1-byte
                // unsigned char.
                const auto status = thdr_rdr.read_raw<tdsl::detail::tds_message_status>();
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
                    return processed_tds_message_count;
                }

                // Length field includes the header length too, so subtract it
                const auto packet_data_size = length - k_tds_hdr_len;
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
                    TDSL_ASSERT_MSG(0, "The receive buffer does not contain expected amount of "
                                       "bytes, something is wrong!");
                    return processed_tds_message_count;
                }

                // TODO: check if downstream consumed the message
                // TODO: Check if we got enough space to pull the next message

                if (packet_data_cb) {
                    const auto needed_bytes = packet_data_cb.invoke(message_type, nmsg_rdr);
                    TDSL_DEBUG_PRINTLN("network_impl_base::handle_tds_response(...) -> msg_cb_ctx, "
                                       "need byte(s) value:%d",
                                       needed_bytes);
                    (void) needed_bytes;
                }

                eom_flag = status.end_of_message;
            } while (processed_tds_message_count++, not eom_flag);
            return processed_tds_message_count;
        }

        void do_send_tds_pdu() {
            //
            // send_buffer will contain the data to be sent
            // split PDU so each TDS packet will be `packet_size` bytes

            // write header, append packet_size - 8 bytes, EOM=false
        }

        /**
         * Set TDS packet data callback
         *
         * @param [in] user_ptr uptr
         * @param [in] cb New callback
         */
        TDSL_SYMBOL_VISIBLE void
        register_packet_data_callback(void * user_ptr, tds_packet_data_callback::function_type cb) {
            packet_data_cb.set(user_ptr, cb);
        }

    private:
        tds_packet_data_callback packet_data_cb{};
        // Negotiated TDS packet size
        tdsl::uint16_t tds_packet_size = {4096};
    };
}} // namespace tdsl::net