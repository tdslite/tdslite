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

#include <tdslite/net/base/network_io_base.hpp>

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
        template <tdsl::uint32_t BufSize>
        inline tdsl_netimpl_arduino(tdsl::uint8_t (&network_io_buffer) [BufSize],
                                    EthernetClientType ec = {}) :
            client(TDSL_MOVE(ec)) {
            this->network_buffer = tdsl::tdsl_buffer_object{network_io_buffer};
        }

        // --------------------------------------------------------------------------------

        /**
         * Construct a new tdsl driver object
         *
         * @param [in] network_io_buffer Network I/O buffer
         */
        template <typename Z>
        inline tdsl_netimpl_arduino(tdsl::byte_span network_io_buffer, EthernetClientType ec = {}) :
            client(TDSL_MOVE(ec)) {
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
        TDSL_SYMBOL_VISIBLE int do_connect(T target, tdsl::uint16_t port) {
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
                return -99; // FIXME: proper error code
            }

            // Retry up to MAX_CONNECT_ATTEMPTS times.
            while (retries--) {
                TDSL_DEBUG_PRINTLN("... attempting to connect to %s:%d, %d retries remaining ...",
                                   destination_host.data(), port, retries);
                cr = client.connect(destination_host.data(), port);
                if (cr == 1) {
                    TDSL_DEBUG_PRINTLN("... connected, %d --> %d.%d.%d.%d:%d ...",
                                       client.localPort(), client.remoteIP() [0],
                                       client.remoteIP() [1], client.remoteIP() [2],
                                       client.remoteIP() [3], client.remotePort());
                    break;
                }

                TDSL_DEBUG_PRINTLN("... connection attempt failed (%d) ...", cr);
                delay(3000);
            }

            // Reset the network buffer so it's ready to
            // use by its real purpose
            this->network_buffer.get_writer()->reset();

            if (cr == 1) {
                return 0;
            }
            return cr;
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

            if (transfer_exactly > dst_buf.size_bytes()) {
                return network_io_result::unexpected(-3); // error case
            }
            const auto wfb_r = wait_for_bytes(transfer_exactly);
            if (wfb_r) {
                // Transfer `exactly` @p transfer_exactly bytes
                client.read(dst_buf.data(), transfer_exactly);
                return transfer_exactly;
            }

            // There is an error, we should handle it appropriately
            TDSL_DEBUG_PRINTLN("tdsl_netimpl_arduino::do_recv(...) -> error, wait for bytes value "
                               "(%d) < (%d) aborting and "
                               "disconnecting",
                               wfb_r.error(), transfer_exactly);

            do_disconnect();
            return network_io_result::unexpected(-1); // error case
        }

        // --------------------------------------------------------------------------------

        TDSL_SYMBOL_VISIBLE auto do_recv(tdsl::uint32_t transfer_exactly) noexcept
            -> network_io_result {
            auto writer          = this->network_buffer.get_writer();
            const auto rem_space = writer->remaining_bytes();

            if (transfer_exactly > rem_space) {
                TDSL_DEBUG_PRINTLN("tdsl_netimpl_arduino::do_recv(...) -> error, not enough "
                                   "space in recv buffer (%u vs %ld)",
                                   transfer_exactly, rem_space);
                TDSL_ASSERT(0);
                return network_io_result::unexpected(-2);
            }
            auto free_space_span = writer->free_span();
            auto result          = do_recv(transfer_exactly, free_space_span);

            if (result) {
                writer->advance(static_cast<tdsl::int32_t>(transfer_exactly));
            }

            return result;
        }

    private:
        /**
         * Wait for bytes to arrive
         *
         * @param [in] bytes_need Amount of bytes needed
         * @param [in] poll_interval How often client.available() should be checked
         * @param [in] timeout When to give up
         *
         * @returns amount of bytes available when successful
         * @returns -1 if client is disconnected during poll
         * @returns -2 if operation could not be completed in time
         */
        inline network_io_result wait_for_bytes(tdsl::size_t bytes_need,
                                                tdsl::uint32_t poll_interval = 300,
                                                tdsl::uint32_t timeout       = 30000) noexcept {
            const tdsl::uint32_t wait_till = millis() + timeout;

            while (client.connected()) {
                delay(poll_interval / 2);
                const auto bytes_avail = client.available();

                if (bytes_avail < 0) {
                    return -1;
                }

                if (static_cast<tdsl::size_t>(bytes_avail) >= bytes_need) {
                    return static_cast<tdsl::size_t>(bytes_avail);
                }
                if (millis() >= wait_till) {
                    // timeout
                    TDSL_DEBUG_PRINTLN(
                        "tdsl_netimpl_arduino::wait_for_bytes(...) -> error, time out!");
                    return network_io_result::unexpected(-2);
                }
                TDSL_DEBUG_PRINTLN("tdsl_netimpl_arduino::wait_for_bytes(...) --> still polling "
                                   "[avail:`%d`, need:`%ld`]",
                                   bytes_avail, bytes_need);
                delay(poll_interval / 2);
            }

            TDSL_DEBUG_PRINTLN("tdsl_netimpl_arduino::wait_for_bytes(...) -> error, disconnected!");
            do_disconnect();
            return network_io_result::unexpected(-1);
        }

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