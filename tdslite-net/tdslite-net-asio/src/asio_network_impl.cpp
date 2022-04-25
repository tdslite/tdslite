/**
 * _________________________________________________
 *
 * @file   mock_network_impl.cpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   20.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#include <tdslite/net/asio/asio_network_impl.hpp>
#include <tdslite/util/tdsl_hex_dump.hpp>

#define BOOST_ASIO_ENABLE_HANDLER_TRACKING 1
#include <boost/asio.hpp>
#include <thread>

using io_context_t   = boost::asio::io_context;
using work_guard_t   = boost::asio::executor_work_guard<io_context_t::executor_type>;
using tcp_t          = boost::asio::ip::tcp;
using tcp_socket_t   = tcp_t::socket;
using tcp_resolver_t = tcp_t::resolver;

namespace {

    auto as_ctx(std::shared_ptr<void> & v) noexcept -> io_context_t * {
        return reinterpret_cast<io_context_t *>(v.get());
    }

    auto as_guard(std::shared_ptr<void> & v) noexcept -> work_guard_t * {
        return reinterpret_cast<work_guard_t *>(v.get());
    }

    auto as_socket(std::shared_ptr<void> & v) noexcept -> tcp_socket_t * {
        return reinterpret_cast<tcp_socket_t *>(v.get());
    }

    auto as_thread(std::shared_ptr<void> & v) noexcept -> std::thread * {
        return reinterpret_cast<std::thread *>(v.get());
    }

    auto as_resolver(std::shared_ptr<void> & v) noexcept -> tcp_resolver_t * {
        return reinterpret_cast<tcp_resolver_t *>(v.get());
    }

} // namespace

namespace tdsl { namespace net {
    asio_network_impl::asio_network_impl() {
        recv_buffer.resize(8192);
        io_context            = std::make_shared<io_context_t>(1);
        io_context_work_guard = std::make_shared<work_guard_t>(boost::asio::make_work_guard(*as_ctx(io_context)));
        worker_thread         = std::make_shared<std::thread>([this]() {
            as_ctx(io_context)->run();
            printf("exited\n");
        });
        as_ctx(io_context)->post([]() {
            printf("called\n");
        });
        // boost::asio::post();
    }
    asio_network_impl::~asio_network_impl() {
        printf("destr\n");
        io_context_work_guard.reset();
        as_ctx(io_context)->stop();
        as_thread(worker_thread)->join();
    }

    int asio_network_impl::do_connect(tdsl::span<const char> target, tdsl::uint16_t port) {

        enum e_result : std::int32_t
        {
            in_progress          = 0,
            socket_already_alive = -1,
            resolve_in_flight    = -2
        };

        if (socket_handle) {
            printf("exit_already_alive\n");
            return e_result::socket_already_alive;
        }

        if (flags.resolve_in_flight) {
            printf("exit_resolve_in_flight\n");
            return e_result::resolve_in_flight;
        }

        // Let's try to resolve the given address first.
        resolver = std::make_shared<tcp_resolver_t>(*as_ctx(io_context));
        std::string host{target.data(), target.size_bytes()};
        std::string service = std::to_string(port);
        tcp_resolver_t::query q(host, service);
        flags.resolve_in_flight.store(true);
        // printf("aa\n");
        as_resolver(resolver)->async_resolve(q, [this](boost::system::error_code ec, tcp_resolver_t::results_type res) {
            // Check whether the resolver has succeeded to resolve
            printf("%s\n", ec.to_string().c_str());
            if (not ec) {
                printf("resolve ok\n");
                // Prime the socket handle
                auto sock = std::make_shared<tcp_socket_t>(*as_ctx(io_context));
                // Attempt to connect to each resolve result, in order

                for (const auto & re : res) {
                    // We're in completion handler so no need to re-dispatch as asynchronous operation
                    // (i.e. async_connect)
                    printf("attempting to connect %s:%s\n", re.host_name().c_str(), re.service_name().c_str());
                    sock->connect(re.endpoint(), ec);
                    if (not ec) {
                        printf("connected\n");
                        // Connected, initiate async_receive
                        socket_handle = std::move(sock);
                        dispatch_receive();
                        break;
                    }
                }
            }
            else {
                // Otherwise, we got a resolve failure error in our hands.
                // TODO: Report
                printf("resolve failed\n");
            }
            flags.resolve_in_flight.store(false);
        });

        // async_in_progress
        printf("exit_in_progress\n");
        return e_result::in_progress;
    }

    void asio_network_impl::dispatch_receive() {
        TDSLITE_ASSERT(socket_handle);
        // Re-invoke receive
        as_socket(socket_handle)->async_receive(boost::asio::buffer(recv_buffer), [this](boost::system::error_code ec, std::size_t amount) {
            if (not ec) {
                on_recv(amount);
                return;
            }

            // There is an error, we should handle it appropriately
        });
    }

    void asio_network_impl::on_recv(tdsl::uint32_t amount) {
        if (amount == 0) {
            // Disconnected
            do_disconnect();
            return;
        }
        // Process the buffer
        printf("Received packet:\n");
        util::hexdump(recv_buffer.data(), amount);

        if (recv_cb_ctx.callback) {
            tdsl::span<const tdsl::uint8_t> rd(recv_buffer.data(), amount);
            recv_cb_ctx.callback(recv_cb_ctx.user_ptr, rd);
        }
        dispatch_receive();
    }

    int asio_network_impl::do_disconnect() noexcept {
        enum e_result : std::int32_t
        {
            success          = 0,
            socket_not_alive = -1,
        };
        if (not socket_handle) {
            return e_result::socket_not_alive;
        }
        printf("disco called\n");
        boost::system::error_code ec;
        // Close the socket
        as_socket(socket_handle)->close(ec);
        // Destroy the socket handle
        socket_handle.reset();
        return e_result::success;
    }

    int asio_network_impl::do_send(void) noexcept {
        enum e_result : std::int32_t
        {
            in_progress    = 0,
            send_in_flight = -1,
        };

        if (flags.send_in_flight) {
            return e_result::send_in_flight;
        }
        flags.send_in_flight.store(true);
        as_socket(socket_handle)->async_send(boost::asio::buffer(send_buffer), [this](boost::system::error_code ec, std::size_t amount) {
            if (not ec) {
                printf("sent %ld byte(s)!\n", amount);
            }

            switch (ec.value()) {
                case 0: // success
                case boost::asio::error::in_progress:
                    break;
                case boost::asio::error::operation_aborted:
                    flags.send_in_flight.store(false);
                    return;
                default: {
                    do_disconnect();
                    return;
                } break;
            }

            flags.send_in_flight.store(false);
        });
        return e_result::in_progress;
    }
}} // namespace tdsl::net
