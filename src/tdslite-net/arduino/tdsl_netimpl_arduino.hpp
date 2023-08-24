/**
 * ____________________________________________________
 * Network implementation compatible with Official arduino
 * ethernet library's EthernetClient. Also compatible with
 * other implementations that implements the same interface.
 * (e.g. WiFiClient)
 *
 * @file   tdsl_netimpl_arduino.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   20.04.2022
 *
 * SPDX-License-Identifier: MIT
 * ____________________________________________________
 */

#ifndef TDSL_NET_NETIMPL_ARDUINO_HPP
#define TDSL_NET_NETIMPL_ARDUINO_HPP

#include <tdslite-net/base/network_io_base.hpp>

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_expected.hpp>

namespace tdsl { namespace net {

    /**
     * Synchronous networking code for Arduino-like devices.
     *
     * This implementation does not depend on any concrete type and will
     * work with any EthernetClientType class that provides the following
     * public API interface:
     *
     * - A write function that can be called with `const unsigned char* buf, unsigned long len`
     *   * Should write the content to the network buffer
     * - connect() function that can be called with `const char* host, unsigned short port`
     *   * Must try to establish connection to host:port
     * - read() function that can be called with `unsigned char* buf, unsigned long amount`
     *   * Transfers 'exactly' amount bytes to the 'buf'. 'buf' is guaranteed to have 'amount'
     *     bytes of space.
     * - `stop()` function
     *
     * Also, the implementation depends on following global free functions as well:
     *
     *  * millis()   -> milliseconds obtained from a steady clock
     *  * delay(ms)  -> must put current caller to sleep for at least ms milliseconds
     */
    template <typename EthernetClientType>
    struct tdsl_netimpl_arduino : public network_io_base<tdsl_netimpl_arduino<EthernetClientType>> {

        using network_io_result =
            typename network_io_base<tdsl_netimpl_arduino<EthernetClientType>>::network_io_result;

        // --------------------------------------------------------------------------------

        /**
         * Construct a new tdsl driver object
         *
         * @param [in] network_io_buffer Network I/O buffer
         */
        template <tdsl::uint32_t BufSize, typename... Args>
        inline tdsl_netimpl_arduino(tdsl::uint8_t (&network_io_buffer) [BufSize], Args &&... args) :
            client(TDSL_FORWARD(args)...) {
            this->network_buffer = tdsl::tdsl_buffer_object{network_io_buffer};
        }

        // --------------------------------------------------------------------------------

        /**
         * Construct a new tdsl driver object
         *
         * @param [in] network_io_buffer Network I/O buffer
         */
        template <typename Z, typename... Args>
        inline tdsl_netimpl_arduino(tdsl::byte_span network_io_buffer, Args &&... args) :
            client(TDSL_FORWARD(args)...) {
            this->network_buffer = tdsl::tdsl_buffer_object{network_io_buffer};
        }

        // --------------------------------------------------------------------------------

        /**
         * Try to connect to the target endpoint @p target : @p port
         *
         * @param [in] target Host name or IP address
         * @param [in] port Port number
         * @return 0 if connection is successful, implementation-specific error code
         *         otherwise.
         */
        template <typename T>
        TDSL_SYMBOL_VISIBLE tdsl::expected<tdsl::traits::true_type, int>
        do_connect(T target, tdsl::uint16_t port) {
            // Disconnect if already connected
            do_disconnect();

            int cr      = 0;
            int retries = 10;

            // There may be residue data in network buffer
            // Reset it to ensure a clean start
            this->network_buffer.get_writer()->reset();

            tdsl::char_view destination_host{};

            // Given that @p target can be a progmem string,
            // we need to ensure that it is in SRAM before passing
            // it to `connect` function. For that, instead of allocating
            // a new buffer, we use the network buffer since it's sufficiently
            // large free space not yet in use for anything.

            if (target.size_bytes() <= this->network_buffer.get_writer()->remaining_bytes()) {

                auto w = this->network_buffer.get_writer();
                for (auto c : target) {
                    auto wr = w->write(c);
                    (void) wr;
                }
                destination_host = w->inuse_span().template rebind_cast<const char>();
            }
            else {
                return tdsl::unexpected(-99); // FIXME: proper error code
            }

            // Retry up to MAX_CONNECT_ATTEMPTS times.
            while (retries--) {
                TDSL_DEBUG_PRINTLN("... attempting to connect to %s:%d, %d retries remaining ...",
                                   destination_host.data(), port, retries);
                cr = client.connect(destination_host.data(), port);
                if (cr == 1) {
                    TDSL_DEBUG_PRINTLN("... connected to --> %d.%d.%d.%d:%d ...",
                                       client.remoteIP() [0], client.remoteIP() [1],
                                       client.remoteIP() [2], client.remoteIP() [3],
                                       client.remotePort());
                    break;
                }

                TDSL_DEBUG_PRINTLN("... connection attempt failed (%d) ...", cr);
                delay(3000);
            }

            // Reset the network buffer so it's ready to
            // use by its real purpose
            this->network_buffer.get_writer()->reset();

            if (cr == 1) {
                return tdsl::traits::true_type{};
            }
            return tdsl::unexpected(cr);
        }

        // --------------------------------------------------------------------------------

        TDSL_SYMBOL_VISIBLE tdsl::int32_t do_disconnect() {
            client.stop();
            return 0;
        }

        // --------------------------------------------------------------------------------

        /**
         * Send byte_views @p header and @p bufs sequentially to the connected endpoint
         *
         * (scatter-gather I/O)
         */
        TDSL_SYMBOL_VISIBLE void do_send(byte_view header, byte_view message) noexcept {
            client.write(header.data(), header.size_bytes());
            client.write(message.data(), message.size_bytes());
            client.flush();
        }

        // --------------------------------------------------------------------------------

        /**
         * Read exactly @p dst_buf.size() bytes from socket
         *
         * @param [in] dst_buf Destination
         */
        TDSL_SYMBOL_VISIBLE auto do_recv(tdsl::uint32_t transfer_exactly, byte_span dst_buf)
            -> network_io_result {

            enum class errc : int
            {
                disconnected           = -1,
                timeout                = -2,
                not_enough_capacity    = -3,
                unexpected_read_amount = -99
            };

            const tdsl::uint32_t poll_interval = 300, timeout = 30000;

            if (transfer_exactly > dst_buf.size_bytes()) {
                // Destination buffer does not have the capacity
                return tdsl::unexpected(static_cast<int>(errc::not_enough_capacity));
            }

            // When we should give up on trying to receive
            const tdsl::uint32_t wait_till = millis() + timeout;

            // amount of bytes we've been able to
            // pull from the client so far
            tdsl::uint32_t bytes_recvd     = {0};

            do {

                if (not client.available()) {
                    delay(poll_interval);
                }
                else {

                    const auto amount_demanded = transfer_exactly - bytes_recvd;

                    // Read as much data as we can right now.
                    const auto read_amount =
                        client.read(dst_buf.data() + bytes_recvd, amount_demanded);

                    TDSL_TRACE_PRINTLN(
                        "tdsl_netimpl_arduino::do_recv(...) -> read amount: %u, demanded:%u",
                        read_amount, amount_demanded);

                    if (read_amount == 0) {
                        TDSL_DEBUG_PRINTLN(
                            "tdsl_netimpl_arduino::do_recv(...) -> ret 0, disconnected");
                        // disconnected
                        do_disconnect();
                        return tdsl::unexpected(static_cast<int>(errc::disconnected)); // error case
                    }
                    else if (read_amount < 0) {
                        TDSL_TRACE_PRINTLN(
                            "tdsl_netimpl_arduino::do_recv(...) -> ret <0, no data avail, waiting");
                        // No data available, so wait for some time
                        delay(poll_interval);
                    }
                    else {
                        if (static_cast<tdsl::uint32_t>(read_amount) > amount_demanded) {
                            TDSL_ASSERT(false);
                            // This cannot happen in a normal implementation
                            // i.e. the client's read() function is buggy.
                            return tdsl::unexpected(static_cast<int>(errc::unexpected_read_amount));
                        }
                        TDSL_ASSERT((bytes_recvd + read_amount) <= transfer_exactly);
                        // Data received
                        bytes_recvd += read_amount;
                    }
                }

                if (millis() >= wait_till) {
                    // timeout
                    TDSL_DEBUG_PRINTLN("tdsl_netimpl_arduino::do_recv(...) -> error, time out!");
                    return tdsl::unexpected(static_cast<int>(errc::timeout));
                }

            } while (!(bytes_recvd == transfer_exactly));

            TDSL_TRACE_PRINTLN("tdsl_netimpl_arduino::do_recv(...) -> received %u bytes",
                               bytes_recvd);

            TDSL_ASSERT(bytes_recvd == transfer_exactly);
            return bytes_recvd;
        }

        // --------------------------------------------------------------------------------

        TDSL_SYMBOL_VISIBLE auto do_recv(tdsl::uint32_t transfer_exactly) noexcept
            -> network_io_result {
            auto writer          = this->network_buffer.get_writer();
            const auto rem_space = writer->remaining_bytes();

            if (transfer_exactly > rem_space) {
                TDSL_DEBUG_PRINTLN("tdsl_netimpl_arduino::do_recv(...) -> error, not enough "
                                   "space in recv buffer (%u vs " TDSL_SIZET_FORMAT_SPECIFIER ")",
                                   transfer_exactly, rem_space);
                TDSL_ASSERT(0);
                return tdsl::unexpected(-2);
            }
            auto free_space_span = writer->free_span();
            auto result          = do_recv(transfer_exactly, free_space_span);

            if (result) {
                writer->advance(static_cast<tdsl::int32_t>(transfer_exactly));
            }

            return result;
        }

    private:
        // --------------------------------------------------------------------------------

        /**
         * The client instance. Any client type compatible with
         * arduino's EthernetClient interface will work with this
         * class.
         */
        EthernetClientType client;
    };
}} // namespace tdsl::net

#endif