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

#include <tdslite/mock/asio_network_impl.hpp>
#include <tdslite/util/hex_dump.hpp>

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

namespace tdslite { namespace mock {
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

    void asio_network_impl::do_connect(tdslite::span<const char> target, tdslite::uint16_t port) {

        if (socket_handle) {
            printf("exit\n");
            return;
        }

        // Let's try to resolve the given address first.
        resolver = std::make_shared<tcp_resolver_t>(*as_ctx(io_context));
        std::string host{target.data(), target.size_bytes()};
        std::string service = std::to_string(port);
        tcp_resolver_t::query q(host, service);
        // printf("aa\n");
        as_resolver(resolver)->async_resolve(q, [this](boost::system::error_code ec, tcp_resolver_t::results_type res) {
            // Check whether the resolver has succeeded to resolve
            printf("%s\n", ec.to_string().c_str());
            if (not ec) {
                printf("resolve ok\n");
                // Prime the socket handle
                socket_handle = std::make_shared<tcp_socket_t>(*as_ctx(io_context));
                // Attempt to connect to each resolve result, in order
                for (const auto & re : res) {
                    // We're in completion handler so no need to re-dispatch as asynchronous operation
                    // (i.e. async_connect)
                    printf("attempting to connect %s:%s\n", re.host_name().c_str(), re.service_name().c_str());
                    as_socket(socket_handle)->connect(re.endpoint(), ec);
                    if (not ec) {
                        printf("connected\n");
                        // Connected, initiate async_receive
                        as_socket(socket_handle)
                            ->async_receive(boost::asio::buffer(recv_buffer), [this](boost::system::error_code ec, std::size_t amount) {
                                if (not ec) {
                                    on_recv(amount);
                                }
                            });
                        break;
                    }
                }

                return;
            }
            printf("resolve failed\n");

            // Otherwise, we got a resolve failure error in our hands.
            // TODO: Report
        });
    }

    void asio_network_impl::on_recv(tdslite::uint32_t amount) {
        // Process the buffer
        (void) amount;
        printf("Received packet:\n");
        util::hexdump(recv_buffer.data(), amount);
        // Re-invoke receive
        as_socket(socket_handle)->async_receive(boost::asio::buffer(recv_buffer), [this](boost::system::error_code ec, std::size_t amount) {
            if (not ec) {
                on_recv(amount);
            }
        });
    }

    void asio_network_impl::do_disconnect() {
        if (not socket_handle) {
            return;
        }
        printf("disco called\n");
        boost::system::error_code ec;
        as_socket(socket_handle)->close(ec);
        (void) ec;
        // Destroy the socket handle
        socket_handle.reset();
    }

    void asio_network_impl::do_send(void) noexcept {
        as_socket(socket_handle)->async_send(boost::asio::buffer(send_buffer), [this](boost::system::error_code ec, std::size_t amount) {
            if (not ec) {
                printf("sent %ld byte(s)!\n", amount);
            }
        });
    }
}} // namespace tdslite::mock
