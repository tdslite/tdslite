/**
 * _________________________________________________
 *
 * @file   mock_network_impl.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   20.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/detail/tds_span.hpp>
#include <tdslite/detail/tds_macrodef.hpp>
#include <iterator>
#include <vector>
#include <memory>

namespace tdslite { namespace mock {

    /**
     * Mock network implementation for tdslite
     */
    struct asio_network_impl {

        TDSLITE_SYMBOL_VISIBLE asio_network_impl();
        TDSLITE_SYMBOL_VISIBLE ~asio_network_impl();

        /**
         *
         *
         * @param target
         */
        TDSLITE_SYMBOL_VISIBLE void do_connect(tdslite::span<const char> target, tdslite::uint16_t port);

        /**
         *
         *
         */
        TDSLITE_SYMBOL_VISIBLE void do_disconnect();

        template <typename T>
        inline void do_write(tdslite::span<T> data) noexcept {
            send_buffer.insert(send_buffer.end(), data.begin(), data.end());
        }

        template <typename T>
        inline void do_write(tdslite::uint32_t offset, tdslite::span<T> data) noexcept {

            auto beg = std::next(send_buffer.begin(), offset);
            auto end = std::next(beg, data.size_bytes());
            if (beg >= send_buffer.end() || end > send_buffer.end()) {
                return;
            }

            std::copy(data.begin(), data.end(), beg);
        }

        TDSLITE_SYMBOL_VISIBLE void do_send(void) noexcept;

        TDSLITE_SYMBOL_VISIBLE void on_recv(tdslite::uint32_t amount);

        std::vector<tdslite::uint8_t> send_buffer;

        // The receive buffer
        std::vector<tdslite::uint8_t> recv_buffer;

    private:
        std::shared_ptr<void> worker_thread;
        std::shared_ptr<void> socket_handle;
        std::shared_ptr<void> io_context;
        std::shared_ptr<void> io_context_work_guard;
        std::shared_ptr<void> resolver;
    };

}} // namespace tdslite::mock