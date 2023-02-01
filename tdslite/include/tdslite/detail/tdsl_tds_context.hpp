/**
 * ____________________________________________________
 * The main TDS context.
 *
 * Main context's job is owning the TDS connection and
 * performing the essential packet handling. The handled
 * packet data is delivered to downstream consumers via
 * callback functions.
 *
 * Main context itself implements no specific functionality.
 * It's subcontext's job (i.e. tdsl_login_context,
 * tdsl_command_context) to provide an interface to perform a
 * specific task (e.g. authenticate to a SQL server, execute sql
 * commands). Subcontexts are designed to be constructed &
 * destructed on demand, whereas the main context is kept
 * alive during the entire lifetime of the connection.
 *
 *
 * @file   tdsl_tds_context.hpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TDS_CONTEXT_HPP
#define TDSL_DETAIL_TDS_CONTEXT_HPP

#include <tdslite/detail/tdsl_message_type.hpp>
#include <tdslite/detail/tdsl_message_token_type.hpp>
#include <tdslite/detail/tdsl_envchange_type.hpp>
#include <tdslite/detail/tdsl_callback.hpp>
#include <tdslite/detail/tdsl_data_type.hpp>
#include <tdslite/detail/tdsl_token_handler_result.hpp>
#include <tdslite/detail/tdsl_net_rx_mixin.hpp>
#include <tdslite/detail/tdsl_net_tx_mixin.hpp>
#include <tdslite/detail/tdsl_tds_header.hpp>
#include <tdslite/detail/token/tds_envchange_token.hpp>
#include <tdslite/detail/token/tds_info_token.hpp>
#include <tdslite/detail/token/tds_loginack_token.hpp>
#include <tdslite/detail/token/tds_done_token.hpp>

#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_byte_swap.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_debug_print.hpp>
#include <tdslite/util/tdsl_expected.hpp>

namespace tdsl { namespace detail {

    template <typename NetImpl>
    struct tdsl_driver;

    template <typename NetImpl>
    struct login_context;

    template <typename NetImpl>
    struct command_context;

    /**
     * Base type for all TDS message contexts
     *
     * @tparam NetworkImplementation Network-layer Implementation
     */
    template <typename NetworkImplementation>
    struct tds_context : public NetworkImplementation,
                         public detail::net_rx_mixin<tds_context<NetworkImplementation>>,
                         public detail::net_tx_mixin<tds_context<NetworkImplementation>> {
        using tds_context_type       = tds_context<NetworkImplementation>;
        using sub_token_handler_fn_t = token_handler_result (*)(
            void *, e_tds_message_token_type, tdsl::binary_reader<tdsl::endian::little> &);
        using info_callback_type             = callback<tds_info_token>;
        using envchange_callback_type        = callback<tds_envchange_token>;
        using loginack_callback_type         = callback<tds_login_ack_token>;
        using done_callback_type             = callback<tds_done_token>;
        using subtoken_handler_callback_type = callback<void, sub_token_handler_fn_t>;
        using tx_mixin                       = detail::net_tx_mixin<tds_context_type>;
        using rx_mixin                       = detail::net_rx_mixin<tds_context_type>;

        // --------------------------------------------------------------------------------

        struct {
            // ENVCHANGE token callback
            envchange_callback_type envinfochg               = {};
            // INFO/ERROR token callback
            info_callback_type info                          = {};
            // LOGINACK token callback
            loginack_callback_type loginack                  = {};
            // DONE token callback
            done_callback_type done                          = {};
            // Subtoken handler
            subtoken_handler_callback_type sub_token_handler = {};
        } callbacks;

    private:
        struct {
            // Indicates that the tds_context is authenticated against
            // the connected server.
            bool authenticated : 1;
            bool reserved : 7;
        } flags = {};

    public:
        // --------------------------------------------------------------------------------

        /**
         * Whether the context has authenticated against
         * the connected server or not.
         */
        inline bool is_authenticated() const noexcept {
            return flags.authenticated;
        }

        // --------------------------------------------------------------------------------

        /**
         * Default constructor for tds_context
         */
        template <typename... Args>
        inline tds_context(Args &&... args) noexcept :
            NetworkImplementation(TDSL_FORWARD(args)...) {
            // Register tds_context message handler to network implementation
            this->register_packet_data_callback(&handle_packet_data, this);
        }

    private:
        // --------------------------------------------------------------------------------

        /**
         * Default handler function for TDS messages.
         *
         * Dissects the incoming TDS message and invokes the corresponding message
         * handler function.
         *
         * @param [in] self_optr Opaque pointer to self (tds_context)
         * @param [in] message_type Incoming message type
         * @param [in] nmsg_rdr Reader to the incoming message data
         *
         * @return tdsl::uint32_t Amount of needed bytes to read a complete TDS message, if any.
         *                       The return value would be non-zero only if the reader has
         * partial TDS message data.
         */
        static tdsl::uint32_t
        handle_packet_data(void * self_optr, tdsl::detail::e_tds_message_type message_type,
                           tdsl::binary_reader<tdsl::endian::little> & nmsg_rdr) {
            using msg_type = tdsl::detail::e_tds_message_type;
            auto self      = reinterpret_cast<tds_context_type *>(self_optr);

            switch (message_type) {
                case msg_type::tabular_result: {
                    return self->handle_tabular_result_msg(nmsg_rdr);
                } break;
                default: {
                    TDSL_DEBUG_PRINTLN(
                        "tds_context::handle_msg: unhandled (%zu) bytes of msg with type (%d)",
                        nmsg_rdr.remaining_bytes(), static_cast<int>(message_type));
                } break;
            }

            return 0;
        }

        // --------------------------------------------------------------------------------

        /**
         * Handler for TDS tabular result message type.
         *
         * The function extracts each individual token present
         * in the tabular result message and calls the appropriate
         * handler function for the extracted token.
         *
         * @param [in] msg_rdr Reader to read from
         * @returns
         * Amount of needed bytes to read a complete tabular result message, if any.
         * The return value would be non-zero only if the reader has partial
         * message data.
         */
        inline tdsl::uint32_t
        handle_tabular_result_msg(tdsl::binary_reader<tdsl::endian::little> & msg_rdr) noexcept {
            using e_token_type                   = tdsl::detail::e_tds_message_token_type;

            constexpr int k_min_token_need_bytes = 3;
            // start parsing tokens
            while (msg_rdr.has_bytes(/*amount_of_bytes=*/k_min_token_need_bytes)) {

                // Put a checkpoint first
                // we might need to revert (e.g. fragmented token).
                auto checkpoint       = msg_rdr.checkpoint();
                const auto token_type = static_cast<e_token_type>(msg_rdr.read<tdsl::uint8_t>());

                // Yet again, the protocol designers have decided to not to
                // be consistent with their design. Some of the tokens
                // have size information available, some dont. Sigh...
                // One does not simply "be consistent".
                //
                // In my opinion, it must be an obligation for protocol
                // designers to implement their protocol from scratch,
                // just to see how stupid their decisions are.

                // Possibility #1: Tokens with no size information
                // We expect these handlers to advance the reader position themselves.

                if (callbacks.sub_token_handler) {
                    const auto sth_r = callbacks.sub_token_handler(token_type, msg_rdr);
                    if (not(sth_r.status == token_handler_status::unhandled)) {
                        if (sth_r.needed_bytes) {
                            // Restore message reader back to the checkpoint
                            // so the data we've read until now doesn't
                            // get discarded by the network layer
                            checkpoint.restore();
                            return sth_r.needed_bytes;
                        }
                        continue;
                    }
                }

                auto is_fixed_token_size = [](e_token_type t) -> tdsl::uint32_t {
                    switch (t) {
                        case e_token_type::done:
                        case e_token_type::doneinproc:
                        case e_token_type::doneproc:
                            return 8;
                        case e_token_type::offset:
                        case e_token_type::returnstatus:
                            return 4;
                        default:
                            return 0;
                    }
                };

                tdsl::uint16_t current_token_size = is_fixed_token_size(token_type);
                // Possibility #2: Tokens with size information available
                // beforehand. These are easier to parse since we know
                // how many bytes to expect
                if (current_token_size == 0) {
                    current_token_size = msg_rdr.read<tdsl::uint16_t>();
                }

                // Ensure that we got enough bytes to read the token
                if (not(msg_rdr.has_bytes(current_token_size))) {
                    // reseek to token start so the unread token
                    // data dont get discarded by the network layer
                    checkpoint.restore();
                    return current_token_size - msg_rdr.remaining_bytes();
                }

                tdsl::uint32_t subhandler_nb = {0};
                auto token_reader            = msg_rdr.subreader(current_token_size);
                switch (token_type) {
                    case e_token_type::envchange: {
                        subhandler_nb = handle_envchange_token(token_reader);
                    } break;
                    case e_token_type::error:
                    case e_token_type::info: {
                        subhandler_nb = handle_info_token(token_reader);
                    } break;
                    case e_token_type::done:
                    case e_token_type::doneproc:   // FIXME: Distinguish
                    case e_token_type::doneinproc: // FIXME: Distinguish
                    {
                        subhandler_nb = handle_done_token(token_reader);
                    } break;
                    case e_token_type::loginack: {
                        subhandler_nb = handle_loginack_token(token_reader);
                    } break;

                    default: {
                        TDSL_DEBUG_PRINTLN(
                            "Unhandled TOKEN type [%d (%s)]\n", static_cast<int>(token_type),
                            message_token_type_to_str(static_cast<e_token_type>(token_type)));
                    } break;
                }
                // Advance to the next token
                msg_rdr.advance(current_token_size);

                // If we need more bytes to proceed, ask for more bytes
                if (subhandler_nb > 0) {
                    return subhandler_nb;
                }
            }

            // "All integer types are represented in reverse byte order (little-endian) unless
            // otherwise specified."

            TDSL_ASSERT_MSG(msg_rdr.remaining_bytes() < 3,
                            "There are more than 3 bytes remaining in the reader, something is "
                            "wrong in token handler loop!");

            // We expect here to have consumed all of the reader. If that is not the case
            // that means we got partial data.
            return (msg_rdr.remaining_bytes() == 0
                        ? 0
                        : k_min_token_need_bytes - msg_rdr.remaining_bytes());
        }

        // --------------------------------------------------------------------------------

        /**
         * Handler for ENVCHANGE token type
         *
         * The function parses the given data in @p rr as ENVCHANGE token and calls the
         * environment change callback function, if a callback function is assigned.
         *
         * @param [in] rr Reader to read from
         *
         * @return tdsl::uint32_t Amount of needed bytes to read a complete ERROR token, if any.
         *                        The return value would be non-zero only if the reader has
         * partial token data.
         */
        inline tdsl::uint32_t
        handle_envchange_token(tdsl::binary_reader<tdsl::endian::little> & rr) noexcept {
            using envchange_type = tdsl::detail::e_tds_envchange_type;
            // Read ENVCHANGE type
            const auto ect       = static_cast<envchange_type>(rr.read<tdsl::uint8_t>());
            switch (ect) {

                // Handle envchange types in OLD = B_VARHAR, NEW = B_VARCHAR form
                case envchange_type::database:
                case envchange_type::language:
                case envchange_type::charset:
                case envchange_type::packet_size: {
                    // The reader must have at least 1 bytes
                    TDSL_RETIF_LESS_BYTES(rr, 1);

                    // Read new value
                    const auto nvlen_octets = (rr.read<tdsl::uint8_t>() * 2);
                    TDSL_RETIF_LESS_BYTES(rr, nvlen_octets + 1); // +1 for ovlen
                    const auto nvval        = rr.read(nvlen_octets);
                    // Read old value
                    const auto ovlen_octets = (rr.read<tdsl::uint8_t>() * 2);
                    TDSL_RETIF_LESS_BYTES(rr, ovlen_octets);
                    const auto ovval = rr.read(ovlen_octets);

                    tds_envchange_token envchange_info{};
                    envchange_info.type      = ect;
                    envchange_info.new_value = nvval.rebind_cast<char16_t>();
                    envchange_info.old_value = ovval.rebind_cast<char16_t>();

                    if (ect == envchange_type::packet_size) {
                        auto u16v_to_u16 = [](tdsl::u16char_view v) {
                            const auto as_bv   = v.rebind_cast<const tdsl::uint8_t>();
                            tdsl::uint16_t val = {0};
                            for (tdsl::size_t ix = 0; ix < as_bv.size_bytes(); ix += 2) {
                                val = (val << 3) + (val << 1) + (as_bv [ix] - '0');
                            }

                            // for (char16_t c : v) {
                            // }
                            return val;
                        };

                        /**
                         * Quoting MS-TDS, 2.2.7.9 ENVCHANGE:
                         *
                         * "Type 4 (Packet size) is sent in response to a LOGIN7 message. The
                         * server MAY send a value different from the packet size requested by
                         * the client. That value MUST be greater than or equal to 512 and
                         * smaller than or equal to 32767. Both the client and the server MUST
                         * start using this value for packet size with the message following the
                         * login response message."
                         */
                        this->set_tds_packet_size(u16v_to_u16(envchange_info.new_value));
                    }

                    TDSL_ASSERT_MSG(rr.remaining_bytes() == 0,
                                    "There are unhandled stray bytes in ENVCHANGE token, probably "
                                    "something is wrong!");
                    TDSL_DEBUG_PRINT("received environment change -> type [%d] | ",
                                     static_cast<int>(envchange_info.type));
                    TDSL_DEBUG_PRINT("new_value: [");
                    TDSL_DEBUG_PRINT_U16_AS_MB(envchange_info.new_value);
                    TDSL_DEBUG_PRINT("] | ");
                    TDSL_DEBUG_PRINT("old_value: [");
                    TDSL_DEBUG_PRINT_U16_AS_MB(envchange_info.old_value);
                    TDSL_DEBUG_PRINT("]\n");
                    callbacks.envinfochg(envchange_info);
                    return 0;
                } break;
                default: {
                    TDSL_DEBUG_PRINTLN("Unhandled ENVCHANGE type [%d]", static_cast<int>(ect));
                } break;
            }
            return 0;
        }

        // --------------------------------------------------------------------------------

        /**
         * Handler for INFO/ERROR token types,
         *
         * The function parses the given data in @p rr as INFO/ERROR token and calls the info
         * callback function, if a callback function is assigned.
         *
         * @param [in] rr Reader to read from
         *
         * @return tdsl::uint32_t Amount of needed bytes to read a complete INFO/ERROR token, if
         * any. The return value would be non-zero only if the reader has partial token data.
         */
        inline tdsl::uint32_t handle_info_token(tdsl::binary_reader<tdsl::endian::little> & rr) {
            static constexpr auto k_min_info_bytes = 14;
            if (not rr.has_bytes(k_min_info_bytes)) {
                return k_min_info_bytes - rr.remaining_bytes();
            }

            tds_info_token info_msg{};

            // Number, State, Class, MsgText, ServerName, ProcName, LineNumber
            info_msg.number = rr.read<tdsl::uint32_t>();
            info_msg.state  = rr.read<tdsl::uint8_t>();
            info_msg.class_ = rr.read<tdsl::uint8_t>();

            TDSL_TRY_READ_U16_VARCHAR(msgtext, rr);
            TDSL_TRY_READ_U8_VARCHAR(server_name, rr);
            TDSL_TRY_READ_U8_VARCHAR(proc_name, rr);

            info_msg.line_number = rr.read<tdsl::uint16_t>();

            info_msg.msgtext     = msgtext.rebind_cast<char16_t>();
            info_msg.server_name = server_name.rebind_cast<char16_t>();
            info_msg.proc_name   = proc_name.rebind_cast<char16_t>();

            TDSL_ASSERT_MSG(
                rr.remaining_bytes() == 0,
                "There are unhandled stray bytes in INFO token, probably something is wrong!");

            TDSL_DEBUG_PRINT("received info message -> number [%d] | state [%d] | class [%d] | "
                             "line number [%d] | ",
                             info_msg.number, info_msg.state, info_msg.class_,
                             info_msg.line_number);
            TDSL_DEBUG_PRINT("msgtext: [");
            TDSL_DEBUG_PRINT_U16_AS_MB(info_msg.msgtext);
            TDSL_DEBUG_PRINT("] | ");
            TDSL_DEBUG_PRINT("server_name: [");
            TDSL_DEBUG_PRINT_U16_AS_MB(info_msg.server_name);
            TDSL_DEBUG_PRINT("] | ");
            TDSL_DEBUG_PRINT("proc_name: [");
            TDSL_DEBUG_PRINT_U16_AS_MB(info_msg.proc_name);
            TDSL_DEBUG_PRINT("]\n");

            callbacks.info(info_msg);
            return 0;
        }

        // --------------------------------------------------------------------------------

        /**
         * Handler for LOGINACK token type
         *
         * The function parses the given data in @p rr as LOGINACK token and calls the info
         * callback function, if a callback function is assigned.
         *
         * @param [in] rr Reader to read from
         *
         * @return tdsl::uint32_t Amount of needed bytes to read a complete LOGINACK token, if
         * any. The return value would be non-zero only if the reader has partial token data.
         */
        inline tdsl::uint32_t
        handle_loginack_token(tdsl::binary_reader<tdsl::endian::little> & rr) {
            static constexpr auto k_min_loginack_bytes = 10;
            static constexpr auto k_progver_bytes      = 4;
            if (not rr.has_bytes(k_min_loginack_bytes)) {
                return k_min_loginack_bytes - rr.remaining_bytes();
            }

            tds_login_ack_token token{};

            token.interface   = rr.read<tdsl::uint8_t>();
            token.tds_version = rr.read<tdsl::uint32_t>();
            TDSL_TRY_READ_U8_VARCHAR(progname, rr);

            if (not rr.has_bytes(k_progver_bytes)) {
                return k_progver_bytes - rr.remaining_bytes();
            }

            token.prog_version.maj         = rr.read<tdsl::uint8_t>();
            token.prog_version.min         = rr.read<tdsl::uint8_t>();
            token.prog_version.buildnum_hi = rr.read<tdsl::uint8_t>();
            token.prog_version.buildnum_lo = rr.read<tdsl::uint8_t>();
            token.prog_name                = progname.rebind_cast<char16_t>();

            TDSL_DEBUG_PRINT("received login ack token -> interface [%d] | tds version [0x%x] | ",
                             +token.interface, token.tds_version);
            TDSL_DEBUG_PRINT("prog_name: [");
            TDSL_DEBUG_PRINT_U16_AS_MB(token.prog_name);
            TDSL_DEBUG_PRINT("] | ");
            TDSL_DEBUG_PRINTLN("prog_version: [%d.%d.%d.%d]", token.prog_version.maj,
                               token.prog_version.min, token.prog_version.buildnum_hi,
                               token.prog_version.buildnum_lo);
            callbacks.loginack(token);
            return 0;
        }

        // --------------------------------------------------------------------------------

        /**
         * Handler for DONE token type
         *
         * The function parses the given data in @p rr as DONE token and calls the info
         * callback function, if a callback function is assigned.
         *
         * @param [in] rr Reader to read from
         *
         * @return tdsl::uint32_t Amount of needed bytes to read a complete DONE token, if any.
         *                        The return value would be non-zero only if the reader has
         * partial token data.
         */
        inline tdsl::uint32_t handle_done_token(tdsl::binary_reader<tdsl::endian::little> & rr) {
            // fe
            // 02 00 e0 00 00 00 00 00
            static constexpr auto k_min_done_bytes = 8;
            if (not rr.has_bytes(k_min_done_bytes)) {
                return k_min_done_bytes - rr.remaining_bytes();
            }

            tds_done_token token{};

            token.status         = rr.read<tdsl::uint16_t>();
            token.curcmd         = rr.read<tdsl::uint16_t>();
            token.done_row_count = rr.read<tdsl::uint32_t>();

            TDSL_DEBUG_PRINTLN(
                "received done token -> status [%d] | cur_cmd [%d] | done_row_count [%d]",
                token.status, token.curcmd, token.done_row_count);
            callbacks.done(token);
            return 0;
        }

        friend struct detail::net_rx_mixin<tds_context<NetworkImplementation>>;
        friend struct detail::net_tx_mixin<tds_context<NetworkImplementation>>;
        friend struct tdsl::detail::tdsl_driver<NetworkImplementation>;
        friend struct tdsl::detail::login_context<NetworkImplementation>;
        friend struct tdsl::detail::command_context<NetworkImplementation>;
    };
}} // namespace tdsl::detail

#endif