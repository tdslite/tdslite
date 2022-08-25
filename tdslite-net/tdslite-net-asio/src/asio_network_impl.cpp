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

    auto as_socket(std::shared_ptr<void> & v) noexcept -> tcp_socket_t * {
        return reinterpret_cast<tcp_socket_t *>(v.get());
    }

    auto as_resolver(std::shared_ptr<void> & v) noexcept -> tcp_resolver_t * {
        return reinterpret_cast<tcp_resolver_t *>(v.get());
    }

} // namespace

namespace tdsl { namespace net {
    asio_network_impl::asio_network_impl() {
        TDSLITE_DEBUG_PRINT("asio_network_impl::asio_network_impl() -> constructor call\n");

        recv_buffer.resize(8192);
        io_context            = std::make_shared<io_context_t>(1);
        io_context_work_guard = std::make_shared<work_guard_t>(boost::asio::make_work_guard(*as_ctx(io_context)));
        TDSLITE_DEBUG_PRINT("asio_network_impl::asio_network_impl() -> constructor return\n");
    }
    asio_network_impl::~asio_network_impl() {
        TDSLITE_DEBUG_PRINT("asio_network_impl::~asio_network_impl() -> destructor call\n");
        io_context_work_guard.reset();
        as_ctx(io_context)->stop();

        TDSLITE_DEBUG_PRINT("asio_network_impl::~asio_network_impl() -> destructor return\n");
    }

    int asio_network_impl::do_connect(tdsl::span<const char> target, tdsl::uint16_t port) {

        enum e_result : std::int32_t
        {
            connected            = 0,
            socket_already_alive = -1,
            resolve_failed       = -2,
            connection_failed    = -3
        };

        if (socket_handle) {
            TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> exit, socket already alive\n");
            return e_result::socket_already_alive;
        }

        // Let's try to resolve the given address first.
        resolver = std::make_shared<tcp_resolver_t>(*as_ctx(io_context));
        std::string host{target.data(), target.size_bytes()};
        std::string service = std::to_string(port);
        tcp_resolver_t::query q(host, service);
        boost::system::error_code rec;

        const auto res = as_resolver(resolver)->resolve(q, rec);

        // Check whether the resolver has succeeded to resolve
        if (not rec) {
            TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> resolve ok, %d (%s)\n", rec.value(), rec.what().c_str());
            // Prime the socket handle
            auto sock = std::make_shared<tcp_socket_t>(*as_ctx(io_context));
            // Attempt to connect to each resolve result, in order

            for (const auto & re : res) {

                TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> attempting to connect %s:%s\n", re.host_name().c_str(),
                                    re.service_name().c_str());

                sock->connect(re.endpoint(), rec);
                if (not rec) {

                    TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> connected to %s:%s\n", re.host_name().c_str(),
                                        re.service_name().c_str());
                    socket_handle = std::move(sock);
                    // We're connected to one endpoint, stop trying
                    TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> exit, connected\n");
                    return e_result::connected;
                }
            }

            // Failed to connect to any of the resolved endpoints.
        }
        else {
            // Otherwise, we got a resolve failure error in our hands.
            TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> exit, resolve failed!\n");
            return e_result::resolve_failed;
        }

        TDSLITE_DEBUG_PRINT("asio_network_impl::do_connect(...) -> exit, connection failed\n");
        return e_result::connection_failed;
    }

    void asio_network_impl::do_recv(std::uint32_t transfer_at_least = 1) noexcept {
        TDSLITE_ASSERT(socket_handle);
        boost::system::error_code ec;
        // Re-invoke receive
        const auto read_bytes =
            asio::read(*as_socket(socket_handle), asio::buffer(recv_buffer), asio::transfer_at_least(transfer_at_least), ec);

        if (not ec) {
            on_recv(read_bytes);
            return;
        }

        // There is an error, we should handle it appropriately
        TDSLITE_DEBUG_PRINT("asio_network_impl::dispatch_receive(...) -> error, %d (%s) aborting and disconnecting\n", ec.value(),
                            ec.what().c_str());

        do_disconnect();
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
        auto need_bytes = handle_tds_response(rd);

        if (need_bytes) {
            TDSLITE_DEBUG_PRINT("asio_network_impl::on_recv(...) -> need %d byte(s): \n", need_bytes);
            do_recv(need_bytes);
        }
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
        // conn_cb_ctx.maybe_invoke(e_conection_state::disconnected);
        return e_result::success;
    }

    // command/response

    int asio_network_impl::do_send(void) noexcept {
        enum e_result : std::int32_t
        {
            success      = 0,
            cancelled    = -1,
            disconnected = -2,
        };
        boost::system::error_code ec;
        const auto bytes_written =
            asio::write(*as_socket(socket_handle), boost::asio::const_buffer(send_buffer.data(), send_buffer.size()), ec);

        if (not ec) {
            TDSLITE_DEBUG_PRINT("asio_network_impl::do_send(...) -> sent %ld byte(s), ec %d (%s)\n", bytes_written, ec.value(),
                                ec.what().c_str());
        }

        switch (ec.value()) {
            case 0: // success
            case boost::asio::error::in_progress:
                break;
            case boost::asio::error::operation_aborted:
                return e_result::cancelled;
            default: {
                do_disconnect();
                return e_result::disconnected;
            } break;
        }

        send_buffer.clear();
        TDSLITE_DEBUG_PRINT("asio_network_impl::do_send(...) -> exit, bytes written %ld\n", bytes_written);
        return e_result::success;
    }
}} // namespace tdsl::net
