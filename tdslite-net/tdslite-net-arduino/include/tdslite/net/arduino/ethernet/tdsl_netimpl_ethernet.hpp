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

#include <Ethernet.h>

namespace tdsl { namespace net {

    /**
     * Synchronous ASIO networking code for tdslite
     */
    struct tdsl_netimpl_ethernet : public network_io_base<tdsl_netimpl_ethernet> {

        /**
         * Construct a new tdsl driver object
         *
         * @param [in] network_io_buffer Network I/O buffer
         */
        template <tdsl::uint32_t BufSize>
        inline tdsl_netimpl_ethernet(tdsl::uint8_t (&network_io_buffer) [BufSize],
                                     EthernetClient & ec) :
            client(ec) {
            network_buffer = tdsl::tdsl_buffer_object{network_io_buffer};
        }

        /**
         * Construct a new tdsl driver object
         *
         * @param [in] network_io_buffer Network I/O buffer
         */
        inline tdsl_netimpl_ethernet(tdsl::byte_span network_io_buffer, EthernetClient & ec) :
            client(ec) {
            network_buffer = tdsl::tdsl_buffer_object{network_io_buffer};
        }

        /**
         * Construct a new tdsl driver object
         *
         * @param [in] network_io_buffer Network I/O buffer
         */
        inline tdsl_netimpl_ethernet(tdsl::byte_span network_io_buffer, EthernetClient & ec) :
            client(ec) {
            network_buffer = tdsl::tdsl_buffer_object{network_io_buffer};
        }

        TDSL_SYMBOL_VISIBLE int do_connect(tdsl::char_view target, tdsl::uint16_t port) {
            client.stop();

            int cr      = 0;
            int retries = 3;

            // Retry up to MAX_CONNECT_ATTEMPTS times.
            while (retries--) {
                Serial.println(PSTR("...trying..."));
                cr = client.connect(target.data(), port);

                if (cr == 1) {
                    Serial.println(PSTR("...connected ..."));
                    Serial.print(client.localPort());
                    Serial.print(PSTR(" --> "));
                    Serial.print(client.remoteIP());
                    Serial.print(PSTR(":"));
                    Serial.println(client.remotePort());
                    break;
                }

                Serial.print("...got: ");
                Serial.print(cr);
                Serial.println(" retrying...");

                delay(1000);
            }

            if (cr == 1) {
                return 0;
            }
            return cr;
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
        TDSL_SYMBOL_VISIBLE int do_send(byte_view header, byte_view message) noexcept {
            client.write(header.data(), header.size_bytes());
            client.write(message.data(), message.size_bytes());
            client.flush();
        }

        /**
         * Read exactly @p dst_buf.size() bytes from socket
         *
         * @param [in] dst_buf Destination
         */
        TDSL_SYMBOL_VISIBLE expected<tdsl::uint32_t, tdsl::int32_t>
        do_recv(tdsl::uint32_t transfer_exactly, byte_span dst_buf) {

            if (transfer_exactly > dst_buf.size_bytes()) {
                return unexpected<tdsl::int32_t>{-3}; // error case
            }

            if (wait_for_bytes(transfer_exactly) >= transfer_exactly) {
                // Transfer `exactly` @p transfer_exactly bytes
                client.read(dst_buf.data(), dst_buf.size_bytes());
                // Serial.println("exit");
                Serial.print("avail: ");
                Serial.println(client.available());
                return dst_buf.size_bytes();
            }

            // There is an error, we should handle it appropriately
            TDSL_DEBUG_PRINT(
                PSTR("tdsl_netimpl_asio::dispatch_receive(...) -> error, %d (%s) aborting and "
                     "disconnecting\n"),
                ec.value(), ec.what().c_str());

            // do_disconnect();
            return unexpected<tdsl::int32_t>{-1}; // error case
        }

        TDSL_SYMBOL_VISIBLE expected<tdsl::uint32_t, tdsl::int32_t>
        do_recv(tdsl::uint32_t transfer_exactly) noexcept {
            auto writer                   = network_buffer.get_writer();
            const tdsl::int64_t rem_space = writer->remaining_bytes();

            if (transfer_exactly > rem_space) {
                TDSL_DEBUG_PRINTLN(PSTR("tdsl_netimpl_ethernet::do_recv(...) -> error, not enough "
                                        "space in recv buffer (%u vs %ld)"),
                                   transfer_exactly, rem_space);
                TDSL_ASSERT(0);
                return unexpected<tdsl::int32_t>{-2};
            }
            auto free_space_span = writer->free_span();
            auto result          = do_recv(transfer_exactly, free_space_span);

            if (result) {
                writer->advance(static_cast<tdsl::int32_t>(transfer_exactly));
            }

            return result;
        }

    private:
        tdsl::uint16_t delay_millis = {300};
        tdsl::uint16_t wait_millis  = {3000};

        int wait_for_bytes(int bytes_need) {
            const long wait_till = millis() + delay_millis;
            int num              = 0;
            long now             = 0;

            do {
                now = millis();
                num = client.available();
                if (num < bytes_need)
                    delay(wait_millis);
                else
                    break;
            } while (now < wait_till);

            if (num == 0 && now >= wait_till)
                client.stop();
            return num;
        }

        EthernetClient & client;
    };
}} // namespace tdsl::net

#endif