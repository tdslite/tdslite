/**
 * ____________________________________________________
 * Base class for network I/O implementations
 *
 * @file   network_io_base.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_NET_NETWORK_IO_BASE_HPP
#define TDSL_NET_NETWORK_IO_BASE_HPP

#include <tdslite-net/base/network_io_contract.hpp>

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
#include <tdslite/util/tdsl_type_traits.hpp>

namespace tdsl {

    struct string_view;
    struct wstring_view;
    struct progmem_string_view;

    namespace net {

        /**
         * Base network type
         *
         * @tparam Implementation Concrete network implementation type
         */
        template <typename Implementation>
        struct network_io_base : private network_io_contract<Implementation> {
        private:
            inline Implementation & impl() noexcept {
                return static_cast<Implementation &>(*this);
            }

        public:
            using network_io_result      = expected<tdsl::size_t, tdsl::int32_t>;
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

            network_io_base() noexcept {
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
            template <typename T, traits::enable_when::same_any_of<T, string_view, wstring_view,
                                                                   progmem_string_view> = true>
            TDSL_NODISCARD inline int connect(T host, tdsl::uint16_t port) noexcept {
                return static_cast<Implementation &>(*this).do_connect(host, port);
            }

            /**
             * Receive one, complete TDS PDU.
             */
            tdsl::uint32_t do_receive_tds_pdu() noexcept {
                TDSL_ASSERT_MSG(not(network_buffer.get_underlying_view().data() == nullptr),
                                "The network implementation MUST initialize network_buffer prior "
                                "any network I/O!");

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
                    impl().do_recv(tds_hbuf_s.size_bytes(), tds_hbuf_s);
                    auto thdr_rdr = binary_reader<tdsl::endian::big>{tds_hbuf};

                    // Type defines the type of message. Type is a 1-byte unsigned char.
                    const tdsl::detail::e_tds_message_type message_type =
                        static_cast<tdsl::detail::e_tds_message_type>(
                            thdr_rdr.read<tdsl::uint8_t>());

                    // Status is a bit field used to indicate the message state. Status is a 1-byte
                    // unsigned char.
                    const auto status = thdr_rdr.read_raw<tdsl::detail::tds_message_status>();
                    // Length is the size of the packet including the 8 bytes in the packet header.
                    // It is the number of bytes from the start of this header to the start of the
                    // next packet header. Length is a 2-byte, unsigned short and is represented
                    // in network byte order (big-endian). The Length value MUST be greater than
                    // or equal to 512 bytes and smaller than or equal to 32,767 bytes. The default
                    // value is 4,096 bytes. Starting with TDS 7.3, the Length MUST be the
                    // negotiated packet size when sending a packet from client to server, unless it
                    // is the last packet of a request (that is, the EOM bit in Status is ON) or the
                    // client has not logged in.
                    static constexpr auto k_max_length = 32767;
                    const auto length                  = thdr_rdr.read<tdsl::uint16_t>();
                    if (length < sizeof(detail::tds_header) || length > k_max_length) {
                        TDSL_DEBUG_PRINTLN("invalid tds message length %u", length);
                        // invalid length
                        TDSL_ASSERT_MSG(0, "Invalid tds message length!");
                        return processed_tds_message_count;
                    }

                    // Length field includes the header length too, so subtract it
                    auto packet_data_size = length - sizeof(detail::tds_header);

                    // (mgilor): Instead of trying to pull a complete TDS packet,
                    // we can try to pull remaining space amount from the buffer.
                    // this will deliberately
                    // We might try to pull whatever we got in the buffer.

                    // If we can't pull whole packet, try to pull whatever we can from the
                    // network

                    if (packet_data_size > network_buffer.get_writer()->remaining_bytes()) {
                        TDSL_DEBUG_PRINTLN("Cannot fit complete message into network "
                                           "buffer " TDSL_SIZET_FORMAT_SPECIFIER
                                           " > " TDSL_SIZET_FORMAT_SPECIFIER " "
                                           "will try partial pull",
                                           packet_data_size,
                                           network_buffer.get_writer()->remaining_bytes());
                        do {
                            if (network_buffer.get_writer()->remaining_bytes() == 0) {
                                TDSL_DEBUG_PRINTLN(
                                    "Cannot pull {} byte(s) of data from network, network "
                                    "buffer exhausted!");
                                // There's no point keeping the data around, so reset the buffer
                                network_buffer.get_writer()->reset();
                                return processed_tds_message_count;
                            }
                            const auto recv_bytes =
                                impl().do_recv(network_buffer.get_writer()->remaining_bytes());
                            auto nmsg_rdr           = network_buffer.get_reader();
                            const auto needed_bytes = packet_data_cb(message_type, *nmsg_rdr);
                            (void) needed_bytes;
                            packet_data_size -= recv_bytes;
                        } while (packet_data_size > network_buffer.get_writer()->remaining_bytes());
                    }
                    else {
                        impl().do_recv(packet_data_size); // FIXME: Check for errors
                                                          // This is a netbuf_reader instance.
                        // Read operations on this will be committed
                        // to underlying buffer on object destruction
                        // e.g. nmsg_rdr.read(2) will cause 2 bytes from
                        // the start of the underlying buffer to be removed
                        auto nmsg_rdr = network_buffer.get_reader();

                        // pass current buffer down
                        if (not nmsg_rdr->has_bytes(packet_data_size)) {
                            TDSL_DEBUG_PRINTLN(
                                "network_io_base::do_receive_tds_pdu(...) -> error, receive buffer "
                                "does not contain expected amount of bytes "
                                "(" TDSL_SIZET_FORMAT_SPECIFIER " < " TDSL_SIZET_FORMAT_SPECIFIER
                                ") ",
                                nmsg_rdr->remaining_bytes(), packet_data_size);
                            // this should not happen
                            TDSL_ASSERT_MSG(
                                0, "The receive buffer does not contain expected amount of "
                                   "bytes, something is wrong!");
                            return processed_tds_message_count;
                        }

                        // TODO: check if downstream consumed the message
                        // TODO: Check if we got enough space to pull the next message
                        const auto needed_bytes = packet_data_cb(message_type, *nmsg_rdr);
                        if (needed_bytes) {
                            TDSL_DEBUG_PRINTLN("network_impl_base::do_receive_tds_pdu(...) -> "
                                               "packet_data_cb needs `%d` more bytes",
                                               needed_bytes);
                        }

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
                        TDSL_DEBUG_PRINTLN("Although the EOM is received, receive buffer still "
                                           "contains " TDSL_SIZET_FORMAT_SPECIFIER " bytes of "
                                           "data which means packet handler failed to handle "
                                           "all the data in the "
                                           "message. Discarding the data.",
                                           rbuf_reader->remaining_bytes());
                        // Consume the remaining data
                        rbuf_reader->advance(
                            static_cast<tdsl::ssize_t>(rbuf_reader->remaining_bytes()));
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
                TDSL_ASSERT_MSG(not(network_buffer.get_underlying_view().data() == nullptr),
                                "The network implementation MUST initialize network_buffer "
                                "prior any network I/O!");
                const int k_message_segmentation_size = (tds_packet_size - 8);
                auto buf_rdr                          = network_buffer.get_reader();
                auto fragment_count = (buf_rdr->remaining_bytes() / k_message_segmentation_size);
                do {
                    auto segment_size = buf_rdr->has_bytes(k_message_segmentation_size)
                                            ? k_message_segmentation_size
                                            : buf_rdr->remaining_bytes();
                    auto segment      = buf_rdr->read(segment_size);
                    const tdsl::uint16_t segsize_nbo = host_to_network(static_cast<tdsl::uint16_t>(
                        segment.size_bytes() + sizeof(detail::tds_header)));
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

                    impl().do_send(byte_view{tds_hbuf}, segment);
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
                const auto r = network_buffer.get_writer()->write(data);
                TDSL_ASSERT(r);
                (void) r;
            }

            // --------------------------------------------------------------------------------

            /**
             * Get current write offset
             */
            inline tdsl::size_t do_get_write_offset() noexcept {
                return network_buffer.get_writer()->offset();
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
            inline void do_write(tdsl::size_t offset, tdsl::span<T> data) noexcept {
                const auto r = network_buffer.get_writer()->write(offset, data);
                TDSL_ASSERT(r);
                (void) r;
            }

            // --------------------------------------------------------------------------------

            /**
             * Set TDS packet data callback
             *
             * @param [in] cb New callback
             * @param [in] user_ptr uptr
             */
            TDSL_SYMBOL_VISIBLE inline void
            register_packet_data_callback(tds_packet_data_callback::function_type cb,
                                          void * user_ptr) noexcept {
                packet_data_cb = {cb, user_ptr};
            }

            /**
             * Update negotiated packet size
             *
             * @param [in] value New packet size
             */
            TDSL_SYMBOL_VISIBLE inline void set_tds_packet_size(tdsl::uint16_t value) noexcept {
                if (value > network_buffer.get_underlying_view().size_bytes()) {
                    TDSL_ASSERT_MSG(false, "Negotiated packet size cannot be larger than the "
                                           "network buffer itself!");
                    TDSL_TRAP;
                }

                TDSL_DEBUG_PRINTLN(
                    "network_io_base::set_tds_packet_size(...) -> old [%u], new [%u]",
                    tds_packet_size, value);

                tds_packet_size = value;
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
    } // namespace net
} // namespace tdsl

#endif