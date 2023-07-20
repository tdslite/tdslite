/**
 * ____________________________________________________
 * Network implementation for tdslite using boost::asio.
 *
 * @file   tdsl_netimpl_asio.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   20.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_NET_NETIMPL_ASIO_HPP
#define TDSL_NET_NETIMPL_ASIO_HPP

#include <tdslite-net/base/network_io_base.hpp>

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_expected.hpp>
#include <tdslite/util/tdsl_buffer_object.hpp>

#include <vector>
#include <memory>

namespace tdsl { namespace net {

    /**
     * Synchronous ASIO networking code for tdslite
     */
    struct tdsl_netimpl_asio : public network_io_base<tdsl_netimpl_asio> {

        using network_io_base<tdsl_netimpl_asio>::network_io_result;

        // --------------------------------------------------------------------------------

        /**
         * Default c-tor
         */
        TDSL_SYMBOL_VISIBLE tdsl_netimpl_asio();

        // --------------------------------------------------------------------------------

        /**
         * D-tor
         */
        TDSL_SYMBOL_VISIBLE ~tdsl_netimpl_asio();

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
        TDSL_SYMBOL_VISIBLE tdsl::expected<tdsl::traits::true_type, int>
        do_connect(tdsl::char_view target, tdsl::uint16_t port);

        // --------------------------------------------------------------------------------

        /**
         * Disconnect the socket from the connected endpoint and destroy
         * the socket.
         *
         * @returns 0 if socket is disconnected and the class is ready for re-use
         * @returns -1 if socket is not alive
         */
        TDSL_SYMBOL_VISIBLE tdsl::int32_t do_disconnect() noexcept;

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
        TDSL_SYMBOL_VISIBLE tdsl::int32_t do_send(byte_view header, byte_view message) noexcept;

        // --------------------------------------------------------------------------------

        /**
         * Read exactly @p exact_amount bytes from socket
         * into network buffer.
         *
         * @param [in] transfer_amount Exact amount of bytes to read
         */
        TDSL_SYMBOL_VISIBLE network_io_result do_recv(tdsl::uint32_t transfer_amount) noexcept;

        // --------------------------------------------------------------------------------

        /**
         * Read exactly @p exact_amount bytes from socket
         * into @p dst_buf
         *
         * @param [in] dst_buf Destination
         * @param [in] transfer_amount Exact amount of bytes to read
         */
        TDSL_SYMBOL_VISIBLE network_io_result do_recv(tdsl::uint32_t transfer_amount,
                                                      byte_span dst_buf);

    private:
        // Underlying buffer
        static constexpr tdsl::uint32_t k_buffer_size = {16384};
        std::vector<tdsl::uint8_t> underlying_buffer{std::vector<tdsl::uint8_t>(k_buffer_size)};
        // Type-erased smart pointers to asio-specific stuff
        std::shared_ptr<void> io_context{nullptr};
        std::shared_ptr<void> io_context_work_guard{nullptr};
        std::shared_ptr<void> socket_handle{nullptr};
        std::shared_ptr<void> resolver{nullptr};
    };

}} // namespace tdsl::net

#endif