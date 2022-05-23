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
#include <tdslite/util/tdsl_debug_print.hpp>

#define BOOST_ASIO_ENABLE_HANDLER_TRACKING 1
#include <boost/asio.hpp>
#include <boost/asio/read.hpp>
#include <thread>

namespace asio       = boost::asio;
using io_context_t   = asio::io_context;
using work_guard_t   = asio::executor_work_guard<io_context_t::executor_type>;
using strand_t       = asio::strand<io_context_t::executor_type>;
using tcp_t          = asio::ip::tcp;
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

    auto as_strand(std::shared_ptr<void> & v) noexcept -> strand_t * {
        return reinterpret_cast<strand_t *>(v.get());
    }

} // namespace

namespace tdsl { namespace net {
    asio_network_impl::asio_network_impl() {
        TDSLITE_DEBUG_PRINT("asio_network_impl::asio_network_impl() -> constructor call\n");

        recv_buffer.resize(8192);
        io_context            = std::make_shared<io_context_t>(1);
        io_context_work_guard = std::make_shared<work_guard_t>(boost::asio::make_work_guard(*as_ctx(io_context)));
        worker_thread         = std::make_shared<std::thread>([this]() {
            TDSLITE_DEBUG_PRINT("asio_network_impl::asio_network_impl() -> worker thread spawn\n");
            as_ctx(io_context)->run();
            TDSLITE_DEBUG_PRINT("asio_network_impl::asio_network_impl() -> worker thread exit\n");
        });
        send_strand           = std::make_shared<strand_t>(asio::make_strand(as_ctx(io_context)->get_executor()));
        recv_strand           = std::make_shared<strand_t>(asio::make_strand(as_ctx(io_context)->get_executor()));
        TDSLITE_DEBUG_PRINT("asio_network_impl::asio_network_impl() -> constructor return\n");
    }
    asio_network_impl::~asio_network_impl() {
        TDSLITE_DEBUG_PRINT("asio_network_impl::~asio_network_impl() -> destructor call\n");
        io_context_work_guard.reset();
        as_ctx(io_context)->stop();
        as_thread(worker_thread)->join();
        TDSLITE_DEBUG_PRINT("asio_network_impl::~asio_network_impl() -> destructor return\n");
    }

    int asio_network_impl::do_connect(tdsl::span<const char> target, tdsl::uint16_t port) {

        enum e_result : std::int32_t
        {
            in_progress          = 0,
            socket_already_alive = -1,
            resolve_in_flight    = -2
        };

        if (socket_handle) {
            TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> exit, socket already alive\n");
            return e_result::socket_already_alive;
        }

        if (flags.resolve_in_flight) {
            TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> exit, resolve already in flight\n");
            return e_result::resolve_in_flight;
        }

        // Let's try to resolve the given address first.
        resolver = std::make_shared<tcp_resolver_t>(*as_ctx(io_context));
        std::string host{target.data(), target.size_bytes()};
        std::string service = std::to_string(port);
        tcp_resolver_t::query q(host, service);
        flags.resolve_in_flight.store(true);

        as_resolver(resolver)->async_resolve(q, [this](boost::system::error_code ec, tcp_resolver_t::results_type res) {
            // Check whether the resolver has succeeded to resolve
            if (not ec) {
                TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> async resolve ok, %d (%s)\n", ec.value(), ec.what().c_str());
                // Prime the socket handle
                auto sock = std::make_shared<tcp_socket_t>(*as_ctx(io_context));
                // Attempt to connect to each resolve result, in order

                for (const auto & re : res) {
                    // We're in completion handler so no need to re-dispatch as asynchronous operation
                    // (i.e. async_connect)
                    TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> attempting to connect %s:%s\n", re.host_name().c_str(),
                                        re.service_name().c_str());

                    sock->connect(re.endpoint(), ec);
                    if (not ec) {

                        TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> connected to %s:%s\n", re.host_name().c_str(),
                                            re.service_name().c_str());
                        socket_handle = std::move(sock);
                        // Connected, initiate async_receive
                        dispatch_receive(/*transfer_at_least=*/8);
                        // Invoke connection state change callback
                        conn_cb_ctx.maybe_invoke(e_conection_state::connected);
                        break;
                    }
                }

                // Failed to connect to any of the resolved endpoints.
                conn_cb_ctx.maybe_invoke(e_conection_state::connection_failed);
            }
            else {
                // Otherwise, we got a resolve failure error in our hands.
                // TODO: Report
                TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> async resolve failed!\n");
                conn_cb_ctx.maybe_invoke(e_conection_state::resolve_failed);
            }
            flags.resolve_in_flight.store(false);
        });

        // async_in_progress
        TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> exit, in progress\n");

        conn_cb_ctx.maybe_invoke(e_conection_state::attempt_in_progress);
        return e_result::in_progress;
    }

    void asio_network_impl::dispatch_receive(std::uint32_t transfer_at_least = 1) {
        TDSLITE_ASSERT(socket_handle);

        // Re-invoke receive
        asio::async_read(*as_socket(socket_handle), asio::buffer(recv_buffer), asio::transfer_at_least(transfer_at_least),
                         [this](const boost::system::error_code & ec, std::size_t amount) {
                             if (not ec) {
                                 on_recv(amount);
                                 return;
                             }

                             // There is an error, we should handle it appropriately
                             TDSLITE_DEBUG_PRINT("asio_network_impl::dispatch_receive(...) -> error, %d (%s) aborting and disconnecting\n",
                                                 ec.value(), ec.what().c_str());

                             do_disconnect();
                         });
    }

    void asio_network_impl::on_recv(tdsl::uint32_t amount) {
        if (amount == 0) {
            // Disconnected
            do_disconnect();
            return;
        }
        // Process the buffer
        TDSLITE_DEBUG_PRINT("asio_network_impl::on_recv(...) -> received %d byte(s): \n", amount);
        TDSLITE_DEBUG_HEXDUMP(recv_buffer.data(), amount);
        tdsl::span<const tdsl::uint8_t> rd(recv_buffer.data(), amount);
        const auto need_bytes = handle_tds_response(rd);
        dispatch_receive(need_bytes);
    }

    int asio_network_impl::do_disconnect() noexcept {
        enum e_result : std::int32_t
        {
            success          = 0,
            socket_not_alive = -1,
        };
        if (not socket_handle) {
            TDSLITE_DEBUG_PRINT("asio_network_impl::do_disconnect(...) -> exit, socket not alive\n");
            return e_result::socket_not_alive;
        }

        boost::system::error_code ec;
        // Close the socket
        as_socket(socket_handle)->close(ec);
        // Destroy the socket handle
        socket_handle.reset();
        TDSLITE_DEBUG_PRINT("asio_network_impl::do_disconnect(...) -> exit, success\n");
        conn_cb_ctx.maybe_invoke(e_conection_state::disconnected);
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

        asio::post(*as_strand(send_strand), [this] {
            asio::async_write(*as_socket(socket_handle), boost::asio::buffer(send_buffer),
                              [this](boost::system::error_code ec, std::size_t amount) {
                                  if (not ec) {
                                      TDSLITE_DEBUG_PRINT("asio_network_impl::do_send(...) -> sent %ld byte(s), ec %d (%s)\n", amount,
                                                          ec.value(), ec.what().c_str());
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
                                  send_buffer.clear();
                              });
        });

        TDSLITE_DEBUG_PRINT("asio_network_impl::do_send(...) -> exit, in progress\n");

        return e_result::in_progress;
    }

}} // namespace tdsl::net
