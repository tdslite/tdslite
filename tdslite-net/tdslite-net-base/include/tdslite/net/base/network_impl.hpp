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

#include <tdslite/net/base/network_impl_contract.hpp>

#include <tdslite/detail/tdsl_callback.hpp>
#include <tdslite/detail/tdsl_message_type.hpp>
#include <tdslite/detail/tdsl_message_token_type.hpp>
#include <tdslite/detail/tdsl_envchange_type.hpp>
#include <tdslite/detail/tdsl_packet_handler_result.hpp>
#include <tdslite/detail/tdsl_message_status.hpp>
#include <tdslite/detail/tdsl_tds_header.hpp>

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_debug_print.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_expected.hpp>
#include <tdslite/util/tdsl_buffer_object.hpp>

namespace tdsl { namespace net {

    /**
     * Base network type
     *
     * @tparam ConcreteNetImpl Concrete network implementation type
     */
    template <typename ConcreteNetImpl>
    struct network_impl : private network_impl_contract<ConcreteNetImpl> {
        struct read_exactly {};

        struct read_at_least {};

    private:
        inline void call_recv(tdsl::uint32_t min_amount, read_at_least) {
            static_cast<ConcreteNetImpl &>(*this).do_recv(min_amount, read_at_least{});
        }

        inline void call_recv(tdsl::uint32_t exact_amount, read_exactly) {
            static_cast<ConcreteNetImpl &>(*this).do_recv(exact_amount, read_exactly{});
        }

        inline void call_send(byte_view view) const noexcept {
            static_cast<ConcreteNetImpl &>(*this).do_send(view);
        }

        inline void call_send(byte_view header, byte_view message) noexcept {
            static_cast<ConcreteNetImpl &>(*this).do_send(header, message);
        }

        inline expected<tdsl::uint32_t, tdsl::int32_t> call_read(byte_span dst_buf, read_exactly) {
            return static_cast<ConcreteNetImpl &>(*this).do_read(dst_buf, read_exactly{});
        }

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

        network_impl() noexcept {
            if (tds_packet_size <= 512) {
                TDSL_ASSERT(0);
                TDSL_UNREACHABLE;
            }
        }

        /**
         * Connect to @p host: @p port
         *
         * @param [in] host Hostname or IP address
         * @param [in] port TCP port number
         * @return int
         */
        TDSL_NODISCARD inline int connect(tdsl::char_view host, tdsl::uint16_t port) noexcept {
            return static_cast<ConcreteNetImpl &>(*this).do_connect(host, port);
        }

        /**
         * Receive one, complete TDS PDU.
         */
        tdsl::uint32_t do_receive_tds_pdu() noexcept {
            TDSL_ASSERT_MSG(
                not(network_buffer.get_underlying_view().data() == nullptr),
                "The network implementation MUST initialize network_buffer prior any network I/O!");

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
                tdsl::uint8_t tds_hbuf [sizeof(detail::tds_header)] = {0};
                byte_span tds_hbuf_s{tds_hbuf};

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
                if (length < sizeof(detail::tds_header) || length > k_max_length) {
                    // invalid length
                    TDSL_ASSERT_MSG(0, "Invalid tds message length!");
                    return processed_tds_message_count;
                }

                // Length field includes the header length too, so subtract it
                const auto packet_data_size = length - sizeof(detail::tds_header);
                call_recv(packet_data_size, read_exactly{}); // may throw?

                // This is a netbuf_reader instance.
                // Read operations on this will be committed
                // to underlying buffer on object destruction
                // e.g. nmsg_rdr.read(2) will cause 2 bytes from
                // the start of the underlying buffer to be removed
                auto nmsg_rdr = network_buffer.get_reader();

                // pass current buffer down
                if (not nmsg_rdr->has_bytes(packet_data_size)) {
                    // this should not happen
                    TDSL_ASSERT_MSG(0, "The receive buffer does not contain expected amount of "
                                       "bytes, something is wrong!");
                    return processed_tds_message_count;
                }

                // TODO: check if downstream consumed the message
                // TODO: Check if we got enough space to pull the next message

                if (packet_data_cb) {
                    const auto needed_bytes = packet_data_cb(message_type, *nmsg_rdr);
                    TDSL_DEBUG_PRINTLN("network_impl_base::handle_tds_response(...) -> msg_cb_ctx, "
                                       "need byte(s) value:%d",
                                       needed_bytes);
                    (void) needed_bytes;
                }

                eom_flag = status.end_of_message;
            } while (processed_tds_message_count++, not eom_flag);

            // NOTE: Sometimes, the message contains some parts that
            // not yet be parsed, which results in parsing failure.
            // The unparsed data remains in the receive buffer and
            // affects the consequent responses' parsing. In such case
            // it is for better to flush the receive buffer.
            {
                auto rbuf_reader = network_buffer.get_reader();
                if (rbuf_reader->remaining_bytes()) {
                    TDSL_DEBUG_PRINTLN(
                        "Although the EOM is received, receive buffer still contains %ld bytes of "
                        "data which means packet handler failed to handle all the data in the "
                        "message. Discarding the data.",
                        rbuf_reader->remaining_bytes());
                    // Consume the remaining data
                    rbuf_reader->advance(
                        static_cast<tdsl::int32_t>(rbuf_reader->remaining_bytes()));
                }
            }
            return processed_tds_message_count;
        }

        /**
         * Send contents of the message buffer in one or more TDS PDU's,
         * depending on negotiated packet size.
         *
         * @param [in] mtype The type of the message currently in
         *                   the network buffer
         */
        void do_send_tds_pdu(tdsl::detail::e_tds_message_type mtype) noexcept {
            TDSL_ASSERT_MSG(
                not(network_buffer.get_underlying_view().data() == nullptr),
                "The network implementation MUST initialize network_buffer prior any network I/O!");
            const int k_message_segmentation_size = (tds_packet_size - 8);
            auto buf_rdr                          = network_buffer.get_reader();
            auto fragment_count = (buf_rdr->remaining_bytes() / k_message_segmentation_size);
            do {
                auto segment_size                = buf_rdr->has_bytes(k_message_segmentation_size)
                                                       ? k_message_segmentation_size
                                                       : buf_rdr->remaining_bytes();
                auto segment                     = buf_rdr->read(segment_size);
                const tdsl::uint16_t segsize_nbo = host_to_network(
                    static_cast<tdsl::uint16_t>(segment.size_bytes() + sizeof(detail::tds_header)));
                const tdsl::uint8_t(&segsize_nbo_b) [2] =
                    reinterpret_cast<const tdsl::uint8_t(&) [2]>(segsize_nbo);

                const tdsl::uint8_t tds_hbuf [sizeof(detail::tds_header)] = {
                    static_cast<tdsl::uint8_t>(mtype),
                    static_cast<tdsl::uint8_t>(fragment_count == 0),
                    segsize_nbo_b [0],
                    segsize_nbo_b [1],
                    0x00,
                    0x00,
                    0x00,
                    0x00};
                call_send(byte_view{tds_hbuf}, segment);
            } while (fragment_count--);

            TDSL_ASSERT_MSG(not buf_rdr->has_bytes(1), "Send buffer must be empty after!");
        }

        // --------------------------------------------------------------------------------

        /**
         * Append @p data to network buffer
         *
         * @param [in] data Data to append
         */
        template <typename T>
        inline void do_write(tdsl::span<T> data) noexcept {
            // FIXME: This should be no longer necessary
            TDSL_ASSERT(network_buffer.get_writer()->write(data));
        }

        // --------------------------------------------------------------------------------

        /**
         * Append @p data to network buffer,
         * starting from @p offset
         *
         * @param [in] offset Offset to start from
         * @param [in] data Data to append
         */
        template <typename T>
        inline void do_write(tdsl::uint32_t offset, tdsl::span<T> data) noexcept {
            TDSL_ASSERT(network_buffer.get_writer()->write(offset, data));
        }

        // --------------------------------------------------------------------------------

        /**
         * Set TDS packet data callback
         *
         * @param [in] user_ptr uptr
         * @param [in] cb New callback
         */
        TDSL_SYMBOL_VISIBLE void
        register_packet_data_callback(void * user_ptr, tds_packet_data_callback::function_type cb) {
            packet_data_cb = {user_ptr, cb};
        }

    private:
        // The callback to be invoked for each TDS packet.
        // The callback is invoked in streaming fashion to
        // free occupied space as soon as possible, which
        // enables network driver to process TDS messages
        // where message_len > network_buffer.
        tds_packet_data_callback packet_data_cb{};

        // Negotiated TDS packet size
        // The capacity of @p network_buffer MUST
        // be equal to this value (may be greater).
        // Otherwise, the client may not be able to
        // receive the data from the server.
        tdsl::uint16_t tds_packet_size = {4096};

    protected:
        // The network I/O buffer.
        //
        // Network implementation share a single buffer
        // for sending/receiving network packets since
        // TDS protocol is a request/response based
        // protocol.
        //
        // MUST be initialized by deriving class
        tdsl_buffer_object network_buffer = {};
    };
}} // namespace tdsl::net