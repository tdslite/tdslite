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

#pragma once

#include <tdslite/net/base/network_impl.hpp>

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_expected.hpp>

#include <Ethernet.h>

namespace tdsl { namespace net {

    // API:
    // connect
    // disconnect
    // write data to socket
    // read from socket

    /**
     * Synchronous ASIO networking code for tdslite
     */
    struct tdsl_netimpl_ethernet : public network_impl<tdsl_netimpl_ethernet> {

        // --------------------------------------------------------------------------------

        /**
         * Default c-tor
         */
        TDSL_SYMBOL_VISIBLE tdsl_netimpl_ethernet();

        // --------------------------------------------------------------------------------

        /**
         * D-tor
         */
        TDSL_SYMBOL_VISIBLE ~tdsl_netimpl_ethernet();

        // --------------------------------------------------------------------------------

        /**
         * Connect to the target endpoint @p target : @p port
         *
         * @param [in] target Hostname or IP
         * @param [in] port Port number
         *
         * @returns 0 when asynchronous resolve is dispatched for @p target and @p port
         * @returns -1 when socket associated with network implementation is alive, call @ref
         * do_disconnect first
         * @returns -2 when asynchronous resolve operation of previous @ref do_connect call is still
         * in progress
         */
        TDSL_SYMBOL_VISIBLE int do_connect(tdsl::char_view target, tdsl::uint16_t port);

        // --------------------------------------------------------------------------------

        /**
         * Disconnect the socket from the connected endpoint and destroy
         * the socket.
         *
         * @returns 0 if socket is disconnected and the class is ready for re-use
         * @returns -1 if socket is not alive
         */
        TDSL_SYMBOL_VISIBLE int do_disconnect() noexcept;

        // --------------------------------------------------------------------------------

        /**
         * Send the data in @ref send_buffer to the connected endpoint
         *
         * @returns 0 when asynchronous send is in progress
         * @returns -1 when asynchronous send is not called due to  another
         *           asynchronous send is already in progress
         */
        TDSL_SYMBOL_VISIBLE int do_send(void) noexcept;

        // --------------------------------------------------------------------------------

        /**
         * Send the data in @p buf to the connected endpoint
         *
         * @returns 0 when asynchronous send is in progress
         * @returns -1 when asynchronous send is not called due to  another
         *           asynchronous send is already in progress
         */
        TDSL_SYMBOL_VISIBLE int do_send(byte_view buf) noexcept;

        // --------------------------------------------------------------------------------

        /**
         * Send byte_views @p header and @p bufs sequentially to the connected endpoint
         *
         * (scatter-gather I/O)
         *
         * @returns 0 when asynchronous send is in progress
         * @returns -1 when asynchronous send is not called due to  another
         *           asynchronous send is already in progress
         */
        TDSL_SYMBOL_VISIBLE int do_send(byte_view header, byte_view message) noexcept;

        // --------------------------------------------------------------------------------

        /**
         * Read exactly @p dst_buf.size() bytes from socket
         *
         * @param [in] dst_buf Destination
         */
        TDSL_SYMBOL_VISIBLE expected<tdsl::uint32_t, tdsl::int32_t> do_read(byte_span dst_buf,
                                                                            read_exactly);

        // --------------------------------------------------------------------------------

        /**
         * Dispatch receive on socket
         *
         * @param [in] minimum_amount Minimum amount of bytes to read from the socket
         */
        TDSL_SYMBOL_VISIBLE void do_recv(tdsl::uint32_t minimum_amount, read_at_least) noexcept;

        // --------------------------------------------------------------------------------

        /**
         * Dispatch receive on socket
         *
         * @param [in] exact_amount Exact amount of bytes to read from the socket
         */
        TDSL_SYMBOL_VISIBLE void do_recv(tdsl::uint32_t exact_amount, read_exactly) noexcept;

        // --------------------------------------------------------------------------------
    };

}} // namespace tdsl::net