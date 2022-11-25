/**
 * _________________________________________________
 *
 * @file   tdsl_netimpl_ethernet.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   20.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSL_NET_NETIMPL_ETHERNET_HPP
#define TDSL_NET_NETIMPL_ETHERNET_HPP

#include <tdslite/net/base/network_io_base.hpp>

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_expected.hpp>

namespace tdsl { namespace net {

    /**
     * Synchronous ASIO networking code for tdslite
     */
    template <typename EthernetClientType>
    struct tdsl_netimpl_ethernet
        : public network_io_base<tdsl_netimpl_ethernet<EthernetClientType>> {
        using network_io_result =
            typename network_io_base<tdsl_netimpl_ethernet<EthernetClientType>>::network_io_result;

        /**
         * Construct a new tdsl driver object
         *
         * @param [in] network_io_buffer Network I/O buffer
         */
        template <tdsl::uint32_t BufSize>
        inline tdsl_netimpl_ethernet(tdsl::uint8_t (&network_io_buffer) [BufSize],
                                     EthernetClientType ec = {}) :
            client(TDSL_MOVE(ec)) {
            this->network_buffer = tdsl::tdsl_buffer_object{network_io_buffer};
        }

        /**
         * Construct a new tdsl driver object
         *
         * @param [in] network_io_buffer Network I/O buffer
         */
        template <typename Z>
        inline tdsl_netimpl_ethernet(tdsl::byte_span network_io_buffer,
                                     EthernetClientType ec = {}) :
            client(TDSL_MOVE(ec)) {
            this->network_buffer = tdsl::tdsl_buffer_object{network_io_buffer};
        }

        TDSL_SYMBOL_VISIBLE int do_connect(tdsl::char_view target, tdsl::uint16_t port) {
            client.stop();

            int cr      = 0;
            int retries = 10;

            // Retry up to MAX_CONNECT_ATTEMPTS times.
            while (retries--) {
                // Serial.println("...trying...");
                cr = client.connect(target.data(), port);

                if (cr == 1) {
                    // Serial.println("...connected ...");
                    // Serial.print(client.localPort());
                    // Serial.print(" --> ");
                    // Serial.print(client.remoteIP());
                    // Serial.print(":");
                    // Serial.println(client.remotePort());
                    break;
                }

                // Serial.print("...got: ");
                // Serial.print(cr);
                // Serial.println(" retrying...");

                delay(3000);
            }

            if (cr == 1) {
                return 0;
            }
            return cr;
        }

        TDSL_SYMBOL_VISIBLE tdsl::int32_t do_disconnect() {
            client.stop();
            return 0;
        }

        /**
         * Send byte_views @p header and @p bufs sequentially to the connected endpoint
         *
         * (scatter-gather I/O)
         *
         * @returns 0 when asynchronous send is in progress
         * @returns -1 when asynchronous send is not called due to  another
         *           asynchronous send is already in progress
         */
        TDSL_SYMBOL_VISIBLE void do_send(byte_view header, byte_view message) noexcept {
            client.write(header.data(), header.size_bytes());
            client.write(message.data(), message.size_bytes());
            client.flush();
        }

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
            TDSL_DEBUG_PRINTLN("tdsl_netimpl_ethernet::do_recv(...) -> error, wait for bytes value "
                               "(%d) < (%d) aborting and "
                               "disconnecting",
                               wfb_r.error(), transfer_exactly);

            do_disconnect();
            return network_io_result::unexpected(-1); // error case
        }

        TDSL_SYMBOL_VISIBLE auto do_recv(tdsl::uint32_t transfer_exactly) noexcept
            -> network_io_result {
            auto writer          = this->network_buffer.get_writer();
            const auto rem_space = writer->remaining_bytes();

            if (transfer_exactly > rem_space) {
                TDSL_DEBUG_PRINTLN("tdsl_netimpl_ethernet::do_recv(...) -> error, not enough "
                                   "space in recv buffer (%u vs %ld)",
                                   transfer_exactly, rem_space);
                TDSL_ASSERT(0);
                return network_io_result::unexpected(-2);
            }
            auto free_space_span = writer->free_span();
            auto result          = do_recv(transfer_exactly, free_space_span);

            if (result) {
                TDSL_DEBUG_PRINTLN("advance by %d", transfer_exactly);
                writer->advance(static_cast<tdsl::int32_t>(transfer_exactly));
            }
            else {
                TDSL_DEBUG_PRINTLN("!!result is false!!", transfer_exactly);
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
            const long wait_till = millis() + timeout;

            while (client.connected()) {
                delay(poll_interval / 2);
                const auto bytes_avail = client.available();
                if (bytes_avail >= bytes_need) {
                    return bytes_avail;
                }
                if (millis() >= wait_till) {
                    // timeout
                    TDSL_DEBUG_PRINTLN(
                        "tdsl_netimpl_ethernet::wait_for_bytes(...) -> error, time out!");
                    return network_io_result::unexpected(-2);
                }
                TDSL_DEBUG_PRINTLN(
                    "tdsl_netimpl_ethernet::wait_for_bytes(...) --> still polling %d/%d",
                    bytes_avail, bytes_need);
                delay(poll_interval / 2);
            }

            TDSL_DEBUG_PRINTLN(
                "tdsl_netimpl_ethernet::wait_for_bytes(...) -> error, disconnected!");
            do_disconnect();
            return network_io_result::unexpected(-1);
        }

        EthernetClientType client;
    };
}} // namespace tdsl::net

#endif