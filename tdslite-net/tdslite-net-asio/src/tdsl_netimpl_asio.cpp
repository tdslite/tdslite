/**
 * _________________________________________________
 *
 * @file   tdsl_netimpl_asio.cpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   20.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

// May be enabled for diagnostics:
// #define TDSL_DEBUG_PRINT_ENABLED
// #define BOOST_ASIO_ENABLE_HANDLER_TRACKING 1

#include <tdslite/net/asio/tdsl_netimpl_asio.hpp>
#include <tdslite/util/tdsl_hex_dump.hpp>
#include <tdslite/util/tdsl_debug_print.hpp>

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

    // --------------------------------------------------------------------------------

    auto as_ctx(std::shared_ptr<void> & v) noexcept -> io_context_t * {
        return reinterpret_cast<io_context_t *>(v.get());
    }

    // --------------------------------------------------------------------------------

    auto as_socket(std::shared_ptr<void> & v) noexcept -> tcp_socket_t * {
        return reinterpret_cast<tcp_socket_t *>(v.get());
    }

    // --------------------------------------------------------------------------------

    auto as_resolver(std::shared_ptr<void> & v) noexcept -> tcp_resolver_t * {
        return reinterpret_cast<tcp_resolver_t *>(v.get());
    }

} // namespace

namespace tdsl { namespace net {

        // --------------------------------------------------------------------------------
        // C-tor

        tdsl_netimpl_asio::tdsl_netimpl_asio() {
            TDSL_DEBUG_PRINT("tdsl_netimpl_asio::tdsl_netimpl_asio() -> constructor call\n");
            network_buffer = tdsl_buffer_object{
                underlying_buffer.data(), static_cast<tdsl::uint32_t>(underlying_buffer.size())};
            io_context = std::make_shared<io_context_t>(1);
            io_context_work_guard =
                std::make_shared<work_guard_t>(boost::asio::make_work_guard(*as_ctx(io_context)));
            TDSL_DEBUG_PRINT("tdsl_netimpl_asio::tdsl_netimpl_asio() -> constructor return\n");
        }

        // --------------------------------------------------------------------------------
        // D-tor

        tdsl_netimpl_asio::~tdsl_netimpl_asio() {
            TDSL_DEBUG_PRINT("tdsl_netimpl_asio::~tdsl_netimpl_asio() -> destructor call\n");
            io_context_work_guard.reset();
            as_ctx(io_context)->stop();

            TDSL_DEBUG_PRINT("tdsl_netimpl_asio::~tdsl_netimpl_asio() -> destructor return\n");
        }

        // --------------------------------------------------------------------------------

        tdsl::int32_t tdsl_netimpl_asio::do_connect(tdsl::span<const char> target,
                                                    tdsl::uint16_t port) {

            enum e_result : tdsl::int32_t
            {
                connected            = 0,
                socket_already_alive = -1,
                resolve_failed       = -2,
                connection_failed    = -3
            };

            if (socket_handle) {
                TDSL_DEBUG_PRINT(
                    "tdsl_netimpl_asio::do_connect(...) -> exit, socket already alive\n");
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
                TDSL_DEBUG_PRINT("tdsl_netimpl_asio::do_connect(...) -> resolve ok, %d (%s)\n",
                                 rec.value(), rec.what().c_str());
                // Prime the socket handle
                auto sock = std::make_shared<tcp_socket_t>(*as_ctx(io_context));
                // Attempt to connect to each resolve result, in order

                for (const auto & re : res) {

                    TDSL_DEBUG_PRINT(
                        "tdsl_netimpl_asio::do_connect(...) -> attempting to connect %s:%s\n",
                        re.host_name().c_str(), re.service_name().c_str());

                    sock->connect(re.endpoint(), rec);
                    if (not rec) {

                        TDSL_DEBUG_PRINT(
                            "tdsl_netimpl_asio::do_connect(...) -> connected to %s:%s\n",
                            re.host_name().c_str(), re.service_name().c_str());
                        socket_handle = std::move(sock);
                        // We're connected to one endpoint, stop trying
                        TDSL_DEBUG_PRINT("tdsl_netimpl_asio::do_connect(...) -> exit, connected\n");
                        return e_result::connected;
                    }
                }

                // Failed to connect to any of the resolved endpoints.
            }
            else {
                // Otherwise, we got a resolve failure error in our hands.
                TDSL_DEBUG_PRINT("tdsl_netimpl_asio::do_connect(...) -> exit, resolve failed!\n");
                return e_result::resolve_failed;
            }

            TDSL_DEBUG_PRINT("tdsl_netimpl_asio::do_connect(...) -> exit, connection failed\n");
            return e_result::connection_failed;
        }

        // --------------------------------------------------------------------------------

        tdsl::int32_t tdsl_netimpl_asio::do_send(byte_view header, byte_view message) noexcept {

            enum e_result : tdsl::int32_t
            {
                success      = 0,
                cancelled    = -1,
                disconnected = -2,
            };

            boost::system::error_code ec                        = {};

            const std::array<boost::asio::const_buffer, 2> bufs = {
                boost::asio::const_buffer{header.data(),  header.size() },
                boost::asio::const_buffer{message.data(), message.size()},
            };

            const auto bytes_written = asio::write(*as_socket(socket_handle), bufs, ec);
            if (not ec) {
                TDSL_DEBUG_PRINT("tdsl_netimpl_asio::do_send(byte_view, byte_view) -> sent %u "
                                 "byte(s), ec %d (%s)\n",
                                 bytes_written, ec.value(), ec.what().c_str());
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

            TDSL_DEBUG_PRINTLN(
                "tdsl_netimpl_asio::do_send(byte_view, byte_view) -> exit, bytes written %u",
                bytes_written);
            (void) bytes_written;
            return e_result::success;
        }

        // --------------------------------------------------------------------------------

        expected<tdsl::uint32_t, tdsl::int32_t>
        tdsl_netimpl_asio::do_recv(tdsl::uint32_t transfer_exactly) noexcept {
            TDSL_ASSERT(socket_handle);

            auto writer          = network_buffer.get_writer();

            const auto rem_space = writer->remaining_bytes();

            if (transfer_exactly > rem_space) {
                TDSL_DEBUG_PRINTLN(
                    "tdsl_netimpl_asio::do_recv(tdsl::uint32_t) -> error, not enough "
                    "space in recv buffer (%u vs %ld)",
                    transfer_exactly, rem_space);
                TDSL_ASSERT(0);
                return unexpected<tdsl::int32_t>{-2}; // error case;
            }

            // Retrieve the free space
            auto free_space_span = writer->free_span();
            auto result          = do_recv(transfer_exactly, free_space_span);

            if (result) {
                // asio::read will write `read_bytes` into `free_space_span`.
                // Advance writer's offset to reflect the changes.
                TDSL_ASSERT(writer->advance(*result));
                TDSL_DEBUG_PRINTLN(
                    "tdsl_netimpl_asio::do_recv(...) -> read bytes(%ld), consumable bytes (%u)",
                    *result, writer->inuse_span().size_bytes());
                TDSL_ASSERT(*result == transfer_exactly);
            }

            return result;
        }

        // --------------------------------------------------------------------------------

        expected<tdsl::uint32_t, tdsl::int32_t>
        tdsl_netimpl_asio::do_recv(tdsl::uint32_t transfer_amount, byte_span dst_buf) {
            TDSL_ASSERT(socket_handle);
            boost::system::error_code ec;

            const auto read_bytes = asio::read(*as_socket(socket_handle),
                                               asio::buffer(dst_buf.data(), dst_buf.size_bytes()),
                                               asio::transfer_exactly(transfer_amount), ec);

            if (not ec) {
                return read_bytes;
            }

            // There is an error, we should handle it appropriately
            TDSL_DEBUG_PRINT("tdsl_netimpl_asio::do_recv(byte_span, tdsl::uint32_t) -> error, %d "
                             "(%s) aborting and "
                             "disconnecting\n",
                             ec.value(), ec.what().c_str());

            do_disconnect();
            return unexpected<tdsl::int32_t>{-1}; // error case
        }

        // --------------------------------------------------------------------------------

        tdsl::int32_t tdsl_netimpl_asio::do_disconnect() noexcept {
            enum e_result : tdsl::int32_t
            {
                success          = 0,
                socket_not_alive = -1,
            };

            if (not socket_handle) {
                TDSL_DEBUG_PRINT("tdsl_netimpl_asio::do_disconnect() -> exit, socket not alive\n");
                return e_result::socket_not_alive;
            }

            boost::system::error_code ec;
            // Close the socket
            as_socket(socket_handle)->close(ec);
            // Destroy the socket handle
            socket_handle.reset();
            TDSL_DEBUG_PRINT("tdsl_netimpl_asio::do_disconnect() -> exit, success\n");
            return e_result::success;
        }

}} // namespace tdsl::net
