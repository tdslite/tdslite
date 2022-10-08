/**
 * _________________________________________________
 *
 * @file   asio_network_impl.hpp
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

#include <iterator>
#include <vector>
#include <memory>
#include <atomic>

namespace tdsl { namespace net {

    /**
     * Synchronous ASIO networking code for tdslite
     */
    struct asio_network_impl : public network_impl<asio_network_impl> {

        // --------------------------------------------------------------------------------

        /**
         * Default c-tor
         */
        TDSL_SYMBOL_VISIBLE asio_network_impl();

        // --------------------------------------------------------------------------------

        /**
         * D-tor
         */
        TDSL_SYMBOL_VISIBLE ~asio_network_impl();

        // --------------------------------------------------------------------------------

        inline net_rbuf_reader rbuf_reader() noexcept {
            return net_rbuf_reader{
                *this, byte_view{recv_buffer.data(), recv_buffer_consumable_bytes}
            };
        }

        // --------------------------------------------------------------------------------

        inline net_sbuf_reader sbuf_reader() noexcept {
            return net_sbuf_reader{
                *this,
                byte_view{send_buffer.data(), static_cast<tdsl::uint32_t>(send_buffer.size())}
            };
        }

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
         * Append @p data to send buffer
         *
         * @param [in] data Data to append
         */
        template <typename T>
        inline void do_write(tdsl::span<T> data) noexcept {
            send_buffer.insert(send_buffer.end(), data.begin(), data.end());
        }

        // --------------------------------------------------------------------------------

        /**
         * Append @p data to send buffer, starting from send buffer offset @p offset
         *
         * @param [in] offset Offset to start from
         * @param [in] data Data to append
         */
        template <typename T>
        inline void do_write(tdsl::uint32_t offset, tdsl::span<T> data) noexcept {

            auto beg = std::next(send_buffer.begin(), offset);
            auto end = std::next(beg, data.size_bytes());
            if (beg >= send_buffer.end() || end > send_buffer.end()) {
                return;
            }

            std::copy(data.begin(), data.end(), beg);
        }

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

        /**
         * Remove @p amount bytes, starting from the @p offset, then move remaining bytes
         * to the start of the buffer.
         *
         * @param [in] amount Amount to remove
         * @param [in] offset Offset, from start
         * @return true if successful, false if recv buf does not have @p amount bytes starting from
         * @p offset
         */
        TDSL_SYMBOL_VISIBLE bool do_consume_recv_buf(tdsl::uint32_t amount,
                                                     tdsl::uint32_t offset = 0);

        // --------------------------------------------------------------------------------

        /**
         * Remove @p amount bytes, starting from the @p offset, then move remaining bytes
         * to the start of the buffer.
         *
         * @param [in] amount Amount to remove
         * @param [in] offset Offset, from start
         * @return true if successful, false if send buf does not have @p amount bytes starting from
         * @p offset
         */
        TDSL_SYMBOL_VISIBLE bool do_consume_send_buf(tdsl::uint32_t amount,
                                                     tdsl::uint32_t offset = 0);

    private:
        // The send buffer
        std::vector<tdsl::uint8_t> send_buffer{};
        // The receive buffer
        std::vector<tdsl::uint8_t> recv_buffer{std::vector<tdsl::uint8_t>(8192)};
        // Amount of bytes consumable in receive buffer
        tdsl::uint32_t recv_buffer_consumable_bytes{0};

        // Type-erased smart pointers to asio-specific stuff
        std::shared_ptr<void> io_context{nullptr};
        std::shared_ptr<void> io_context_work_guard{nullptr};
        std::shared_ptr<void> socket_handle{nullptr};
        std::shared_ptr<void> resolver{nullptr};
    };

}} // namespace tdsl::net