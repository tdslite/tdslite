/**
 * _________________________________________________
 *
 * @file   tdsl_tds_context.hpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/detail/tdsl_message_type.hpp>
#include <tdslite/detail/tdsl_message_token_type.hpp>
#include <tdslite/detail/tdsl_envchange_type.hpp>
#include <tdslite/detail/tdsl_callback.hpp>
#include <tdslite/detail/tdsl_data_type.hpp>
#include <tdslite/detail/tdsl_token_handler_result.hpp>
#include <tdslite/detail/tdsl_net_recv_if_mixin.hpp>
#include <tdslite/detail/tdsl_net_send_if_mixin.hpp>
#include <tdslite/detail/token/tdsl_envchange_token.hpp>
#include <tdslite/detail/token/tdsl_info_token.hpp>
#include <tdslite/detail/token/tdsl_loginack_token.hpp>
#include <tdslite/detail/token/tdsl_done_token.hpp>

#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_byte_swap.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_debug_print.hpp>
#include <tdslite/util/tdsl_expected.hpp>

#define TDSL_TRY_READ_VARCHAR(TYPE, VARNAME, READER)                                                                                       \
    const auto VARNAME##_octets = (READER.read<TYPE>() * 2);                                                                               \
    if (not READER.has_bytes(VARNAME##_octets)) {                                                                                          \
        return VARNAME##_octets - READER.remaining_bytes();                                                                                \
    }                                                                                                                                      \
    const auto VARNAME = READER.read(VARNAME##_octets)

#define TDSL_TRY_READ_U16_VARCHAR(VARNAME, READER) TDSL_TRY_READ_VARCHAR(tdsl::uint16_t, VARNAME, READER)
#define TDSL_TRY_READ_U8_VARCHAR(VARNAME, READER)  TDSL_TRY_READ_VARCHAR(tdsl::uint8_t, VARNAME, READER)

namespace tdsl { namespace detail {

    template <typename NetImpl>
    struct login_context;

    /**
     * Base type for all TDS message contexts
     *
     * @tparam NetImpl Network-layer Implementation
     */
    template <typename ConcreteNetImpl>
    struct tds_context : public ConcreteNetImpl,
                         public detail::net_recv_if_mixin<tds_context<ConcreteNetImpl>>,
                         public detail::net_send_if_mixin<tds_context<ConcreteNetImpl>> {
        using tds_context_type = tds_context<ConcreteNetImpl>;
        using xmit_if          = detail::net_send_if_mixin<tds_context_type>;
        using recv_if          = detail::net_recv_if_mixin<tds_context_type>;

        /**
         * The tabular data stream protocol header
         */
        struct tds_header {
            tdsl::uint8_t type;
            tdsl::uint8_t status;
            tdsl::uint16_t length;
            tdsl::uint16_t channel;
            tdsl::uint8_t packet_number;
            tdsl::uint8_t window;
        } TDSL_PACKED;

        using sub_token_handler_fn_t = token_handler_result (*)(void *, e_tds_message_token_type,
                                                                tdsl::binary_reader<tdsl::endian::little> &);

    private:
        // ENVCHANGE token callback
        callback<tds_envchange_token> envinfochg_cb{};
        // INFO/ERROR token callback
        callback<tds_info_token> info_cb{};
        // LOGINACK token callback
        callback<tds_login_ack_token> loginack_cb{};
        // DONE token callback
        callback<tds_done_token> done_cb{};
        // Subtoken handler
        callback<void, sub_token_handler_fn_t> sub_token_handler{};

        struct {
            // Indicates that the tds_context is authenticated against
            // the connected server.
            bool authenticated : 1;
            bool reserved : 7;
        } flags;

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
        tds_context() noexcept {
            // Register tds_context message handler to network implementation
            this->register_packet_data_callback(this, &handle_packet_data);
        }

        // --------------------------------------------------------------------------------

        /**
         * Write common TDS header of the packet
         *
         * @note 16-bit zero value will be put for the `length` field as a placeholder.
         * The real packet length must be substituted via calling @ref put_tds_header_length
         * function afterwards.
         */
        inline void write_tds_header(e_tds_message_type msg_type) noexcept {
            this->template write(static_cast<tdsl::uint8_t>(msg_type)); // Packet type
            this->template write(/*arg=*/0x01_tdsu8);                   // STATUS
            this->template write(/*arg=*/0_tdsu16);                     // Placeholder for length
            this->template write(/*arg=*/0_tdsu32);                     // Channel, Packet ID and Window
        }

        // --------------------------------------------------------------------------------

        /**
         * Put the packet length into TDS packet.
         *
         * @note @ref write_tds_header() function must already be called
         * before
         *
         * @param [in] data_length Length of the data section
         */
        void put_tds_header_length(tdsl::uint16_t data_length) noexcept {
            // Length is the size of the packet inclusing the 8 bytes in the packet header.
            // It is the number of bytes from start of this header to the start of the next packet header.
            // Length is a 2-byte, unsigned short and is represented in network byte order (big-endian).
            this->template write(TDSL_OFFSETOF(tds_header, length),
                                 host_to_network(static_cast<tdsl::uint16_t>(data_length + sizeof(tds_header))));
        }

        // --------------------------------------------------------------------------------

        /**
         * Size of the TDS header
         */
        inline static auto tds_header_size() noexcept -> tdsl::uint32_t {
            return sizeof(tds_header);
        }

        // --------------------------------------------------------------------------------

        /**
         * Default handler function for TDS messages.
         *
         * Dissects the incoming TDS message and invokes the corresponding message
         * handler function.
         *
         * @param [in] self_optr Opaque pointer to self (tds_context)
         * @param [in] mt Incoming message type
         * @param [in] message Incoming message data
         *
         * @return tdsl::uint32_t Amount of needed bytes to read a complete TDS message, if any.
         *                       The return value would be non-zero only if the reader has partial
         *                       TDS message data.
         */
        static tdsl::uint32_t handle_packet_data(void * self_optr, tdsl::detail::e_tds_message_type message_type,
                                                 tdsl::binary_reader<tdsl::endian::little> & nmsg_rdr) {
            using msg_type = tdsl::detail::e_tds_message_type;
            auto self      = reinterpret_cast<tds_context_type *>(self_optr);

            switch (message_type) {
                case msg_type::tabular_result: {
                    return self->handle_tabular_result_msg(nmsg_rdr);
                } break;
                default: {
                    TDSL_DEBUG_PRINTLN("tds_context::handle_msg: unhandled (%ld) bytes of msg with type (%d)", nmsg_rdr.remaining_bytes(),
                                       static_cast<int>(message_type));
                } break;
            }

            return 0;
        }

        // --------------------------------------------------------------------------------

        /**
         * Register ENVCHANGE token callback.
         *
         * The given callback function will be invoked with token info
         * when a ENVCHANGE token is received.
         *
         * The @p user_ptr will be passed as the first argument to callback
         * function
         *
         * @param [in] user_ptr User pointer
         * @param [in] cb Callback function
         */
        TDSL_SYMBOL_VISIBLE void do_register_envchange_token_callback(void * user_ptr, typename decltype(envinfochg_cb)::function_type cb) {
            envinfochg_cb.set(user_ptr, cb);
        }

        // --------------------------------------------------------------------------------

        /**
         * Register INFO/ERROR token callback.
         *
         * The given callback function will be invoked with token info
         * when a INFO or ERROR token is received.
         *
         * The @p user_ptr will be passed as the first argument to callback
         * function
         *
         * @param [in] user_ptr User pointer
         * @param [in] cb Callback function
         */
        TDSL_SYMBOL_VISIBLE void do_register_info_token_callback(void * user_ptr, typename decltype(info_cb)::function_type cb) {
            info_cb.set(user_ptr, cb);
        }

        // --------------------------------------------------------------------------------

        /**
         * Register LOGINACK token callback.
         *
         * The given callback function will be invoked with token info
         * when a LOGINACK token is received.
         *
         * The @p user_ptr will be passed as the first argument to callback
         * function
         *
         * @param [in] user_ptr User pointer
         * @param [in] cb Callback function
         */
        TDSL_SYMBOL_VISIBLE void do_register_loginack_token_callback(void * user_ptr, typename decltype(loginack_cb)::function_type cb) {
            loginack_cb.set(user_ptr, cb);
        }

        // --------------------------------------------------------------------------------

        /**
         * Register DONE token callback.
         *
         * The given callback function will be invoked with token info
         * when a DONE token is received.
         *
         * The @p user_ptr will be passed as the first argument to callback
         * function
         *
         * @param [in] user_ptr User pointer
         * @param [in] cb Callback function
         */
        TDSL_SYMBOL_VISIBLE void do_register_done_token_callback(void * user_ptr, typename decltype(done_cb)::function_type cb) {
            done_cb.set(user_ptr, cb);
        }

        // --------------------------------------------------------------------------------

        /**
         * Register sub token handler function
         *
         * The given function will be invoked first
         * while handling the tokens.
         *
         * The @p user_ptr will be passed as the first argument to callback
         * function
         *
         * @param [in] user_ptr User pointer
         * @param [in] cb Callback function
         */
        TDSL_SYMBOL_VISIBLE void do_register_sub_token_handler(void * user_ptr, typename decltype(sub_token_handler)::function_type cb) {
            sub_token_handler.set(user_ptr, cb);
        }

    private:
        // --------------------------------------------------------------------------------

        /**
         * Handler for TDS tabular result message type.
         *
         * The function extracts each individual token present
         * in the tabular result message and calls the appropriate
         * handler function for the extracted token.
         *
         * @param [in] rr Reader to read from
         * @returns
         * Amount of needed bytes to read a complete tabular result message, if any.
         * The return value would be non-zero only if the reader has partial
         * message data.
         */
        inline tdsl::uint32_t handle_tabular_result_msg(tdsl::binary_reader<tdsl::endian::little> & msg_rdr) noexcept {
            using e_token_type                   = tdsl::detail::e_tds_message_token_type;

            constexpr int k_min_token_need_bytes = 3;
            // start parsing tokens
            while (msg_rdr.has_bytes(/*amount_of_bytes=*/k_min_token_need_bytes)) {

                // Save offset before token read in case
                // we might need to revert (e.g. fragmented token).
                const auto prev_offset = msg_rdr.offset();
                const auto token_type  = static_cast<e_token_type>(msg_rdr.read<tdsl::uint8_t>());

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

                if (sub_token_handler) {
                    const auto sth_r = sub_token_handler.invoke(token_type, msg_rdr);
                    if (not(sth_r.status == token_handler_status::unhandled)) {
                        if (sth_r.needed_bytes) {
                            // reseek to token start so the unread token
                            // data dont get discarded by the network layer
                            msg_rdr.seek(prev_offset);
                            return sth_r.needed_bytes;
                        }
                        continue;
                    }
                }

                tdsl::uint16_t current_token_size = 0;
                // Possibility #2: Tokens with size information available
                // beforehand. These are easier to parse since we know
                // how many bytes to expect
                if (not(token_type == e_token_type::done)) {
                    current_token_size = msg_rdr.read<tdsl::uint16_t>();
                }
                else {
                    current_token_size = {8}; // sizeof done token(fixed)
                }

                // Ensure that we got enough bytes to read the token
                if (not(msg_rdr.has_bytes(current_token_size))) {
                    // reseek to token start so the unread token
                    // data dont get discarded by the network layer
                    msg_rdr.seek(prev_offset);
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
                    case e_token_type::done: {
                        subhandler_nb = handle_done_token(token_reader);
                    } break;
                    case e_token_type::loginack: {
                        subhandler_nb = handle_loginack_token(token_reader);
                    } break;

                    default: {
                        TDSL_DEBUG_PRINTLN("Unhandled TOKEN type [%d (%s)]\n", static_cast<int>(token_type),
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

            // "All integer types are represented in reverse byte order (little-endian) unless otherwise specified."

            TDSL_ASSERT_MSG(msg_rdr.remaining_bytes() < 3,
                            "There are more than 3 bytes remaining in the reader, something is wrong in token handler loop!");

            // We expect here to have consumed all of the reader. If that is not the case
            // that means we got partial data.
            return (msg_rdr.remaining_bytes() == 0 ? 0 : k_min_token_need_bytes - msg_rdr.remaining_bytes());
        }

        // --------------------------------------------------------------------------------

        /**
         * Handler for ENVCHANGE token type
         *
         * The function parses the given data in @p rr as ENVCHANGE token and calls the environment change
         * callback function, if a callback function is assigned.
         *
         * @param [in] rr Reader to read from
         *
         * @return tdsl::uint32_t Amount of needed bytes to read a complete ERROR token, if any.
         *                        The return value would be non-zero only if the reader has partial
         *                        token data.
         */
        inline tdsl::uint32_t handle_envchange_token(tdsl::binary_reader<tdsl::endian::little> & rr) noexcept {
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
                    TDSL_ASSERT_MSG(rr.remaining_bytes() == 0,
                                    "There are unhandled stray bytes in ENVCHANGE token, probably something is wrong!");
                    TDSL_DEBUG_PRINT("received environment change -> type [%d] | ", static_cast<int>(envchange_info.type));
                    TDSL_DEBUG_PRINT("new_value: [");
                    TDSL_DEBUG_PRINT_U16_AS_MB(envchange_info.new_value);
                    TDSL_DEBUG_PRINT("] | ");
                    TDSL_DEBUG_PRINT("old_value: [");
                    TDSL_DEBUG_PRINT_U16_AS_MB(envchange_info.old_value);
                    TDSL_DEBUG_PRINT("]\n");
                    return envinfochg_cb.maybe_invoke(envchange_info);
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
         * @return tdsl::uint32_t Amount of needed bytes to read a complete INFO/ERROR token, if any.
         *                        The return value would be non-zero only if the reader has partial
         *                        token data.
         */
        inline tdsl::uint32_t handle_info_token(tdsl::binary_reader<tdsl::endian::little> & rr) {
            constexpr static auto k_min_info_bytes = 14;
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

            TDSL_ASSERT_MSG(rr.remaining_bytes() == 0, "There are unhandled stray bytes in INFO token, probably something is wrong!");

            TDSL_DEBUG_PRINT("received info message -> number [%d] | state [%d] | class [%d] | line number [%d] | ", info_msg.number,
                             info_msg.state, info_msg.class_, info_msg.line_number);
            TDSL_DEBUG_PRINT("msgtext: [");
            TDSL_DEBUG_PRINT_U16_AS_MB(info_msg.msgtext);
            TDSL_DEBUG_PRINT("] | ");
            TDSL_DEBUG_PRINT("server_name: [");
            TDSL_DEBUG_PRINT_U16_AS_MB(info_msg.server_name);
            TDSL_DEBUG_PRINT("] | ");
            TDSL_DEBUG_PRINT("proc_name: [");
            TDSL_DEBUG_PRINT_U16_AS_MB(info_msg.proc_name);
            TDSL_DEBUG_PRINT("]\n");

            return info_cb.maybe_invoke(info_msg);
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
         * @return tdsl::uint32_t Amount of needed bytes to read a complete LOGINACK token, if any.
         *                        The return value would be non-zero only if the reader has partial
         *                        token data.
         */
        inline tdsl::uint32_t handle_loginack_token(tdsl::binary_reader<tdsl::endian::little> & rr) {
            constexpr static auto k_min_loginack_bytes = 10;
            constexpr static auto k_progver_bytes      = 4;
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

            TDSL_DEBUG_PRINT("received login ack token -> interface [%d] | tds version [0x%x] | ", +token.interface, token.tds_version);
            TDSL_DEBUG_PRINT("prog_name: [");
            TDSL_DEBUG_PRINT_U16_AS_MB(token.prog_name);
            TDSL_DEBUG_PRINT("] | ");
            TDSL_DEBUG_PRINT("prog_version: [%d.%d.%d.%d]", token.prog_version.maj, token.prog_version.min, token.prog_version.buildnum_hi,
                             token.prog_version.buildnum_lo);
            TDSL_DEBUG_PRINT("\n");
            loginack_cb.maybe_invoke(token);
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
         *                        The return value would be non-zero only if the reader has partial
         *                        token data.
         */
        inline tdsl::uint32_t handle_done_token(tdsl::binary_reader<tdsl::endian::little> & rr) {

            constexpr static auto k_min_done_bytes = 8;
            if (not rr.has_bytes(k_min_done_bytes)) {
                return k_min_done_bytes - rr.remaining_bytes();
            }

            tds_done_token token{};

            token.status         = rr.read<tdsl::uint16_t>();
            token.curcmd         = rr.read<tdsl::uint16_t>();
            token.done_row_count = rr.read<tdsl::uint32_t>();

            TDSL_DEBUG_PRINTLN("received done token -> status [%d] | cur_cmd [%d] | done_row_count [%d]", token.status, token.curcmd,
                               token.done_row_count);
            done_cb.maybe_invoke(token);
            return 0;
        }

        friend struct detail::net_recv_if_mixin<tds_context<ConcreteNetImpl>>;
        friend struct detail::net_send_if_mixin<tds_context<ConcreteNetImpl>>;
        friend struct tdsl::detail::login_context<ConcreteNetImpl>;
    };
}} // namespace tdsl::detail