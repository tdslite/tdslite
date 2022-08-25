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
#include <tdslite/detail/tdsl_callback_context.hpp>
#include <tdslite/detail/tdsl_data_type.hpp>
#include <tdslite/detail/tdsl_row.hpp>
#include <tdslite/detail/token/tdsl_envchange_token.hpp>
#include <tdslite/detail/token/tdsl_info_token.hpp>
#include <tdslite/detail/token/tdsl_loginack_token.hpp>
#include <tdslite/detail/token/tdsl_done_token.hpp>
#include <tdslite/detail/token/tdsl_colmetadata_token.hpp>

#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_byte_swap.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_debug_print.hpp>
#include <tdslite/util/tdsl_expected.hpp>

#define TDSLITE_TRY_READ_VARCHAR(TYPE, VARNAME, READER)                                                                                    \
    const auto VARNAME##_octets = (READER.read<TYPE>() * 2);                                                                               \
    if (not READER.has_bytes(VARNAME##_octets)) {                                                                                          \
        return VARNAME##_octets - READER.remaining_bytes();                                                                                \
    }                                                                                                                                      \
    const auto VARNAME = READER.read(VARNAME##_octets)

#define TDSLITE_TRY_READ_U16_VARCHAR(VARNAME, READER) TDSLITE_TRY_READ_VARCHAR(tdsl::uint16_t, VARNAME, READER)
#define TDSLITE_TRY_READ_U8_VARCHAR(VARNAME, READER)  TDSLITE_TRY_READ_VARCHAR(tdsl::uint8_t, VARNAME, READER)

namespace tdsl { namespace detail {

    template <typename NetImpl>
    struct login_context;

    namespace detail {

        /**
         * Network packet transmit context
         *
         * @tparam Derived Derived type (CRTP)
         */
        template <typename Derived>
        struct net_packet_xmit_context {

            template <typename T, traits::enable_if_integral<T> = true>
            inline auto write(T v) noexcept -> void {
                tdsl::span<const tdsl::uint8_t> data(reinterpret_cast<const tdsl::uint8_t *>(&v), sizeof(T));
                write(data);
            }

            template <typename T, traits::enable_if_integral<T> = true>
            inline auto write_be(T v) noexcept -> void {
                write(native_to_be(v));
            }

            template <typename T, traits::enable_if_integral<T> = true>
            inline auto write_le(T v) noexcept -> void {
                write(native_to_le(v));
            }

            template <typename T, traits::enable_if_integral<T> = true>
            inline auto write(tdsl::uint32_t offset, T v) noexcept -> void {
                tdsl::span<const tdsl::uint8_t> data(reinterpret_cast<const tdsl::uint8_t *>(&v), sizeof(T));
                write(offset, data);
            }

            template <typename T, traits::enable_if_integral<T> = true>
            inline auto write_be(tdsl::uint32_t offset, T v) noexcept -> void {
                write(offset, native_to_be(v));
            }

            template <typename T, traits::enable_if_integral<T> = true>
            inline auto write_le(tdsl::uint32_t offset, T v) noexcept -> void {
                write(offset, native_to_le(v));
            }

            template <typename T>
            inline void write(tdsl::uint32_t offset, tdsl::span<T> data) noexcept {
                static_cast<Derived &>(*this).do_write(offset, data);
            }

            template <typename T>
            inline void write(tdsl::span<T> data) noexcept {
                static_cast<Derived &>(*this).do_write(data);
            }

            template <typename... Args>
            inline void send(Args &&... args) noexcept {
                static_cast<Derived &>(*this).do_send(TDSLITE_FORWARD(args)...);
            }
        };

        // --------------------------------------------------------------------------------

        /**
         * Network packet receive context
         *
         * @tparam Derived Derived type (CRTP)
         */
        template <typename Derived>
        struct net_packet_recv_context {
            template <typename... Args>
            inline void recv(Args &&... args) {
                static_cast<Derived &>(*this).do_recv(TDSLITE_FORWARD(args)...);
            }
        };

        // --------------------------------------------------------------------------------

        template <typename T>
        using register_msg_recv_callback_member_fn_t = decltype(traits::declval<T>().register_msg_recv_callback(
            static_cast<void *>(0),
            static_cast<tdsl::uint32_t (*)(void *, tdsl::detail::e_tds_message_type, tdsl::span<const tdsl::uint8_t>)>(0)));

        template <typename T>
        using has_register_msg_recv_member_fn = traits::is_detected<register_msg_recv_callback_member_fn_t, T>;

        // --------------------------------------------------------------------------------

    } // namespace detail

    /**
     * Base type for all TDS message contexts
     *
     * @tparam NetImpl Network-layer Implementation
     */
    template <typename NetImpl>
    struct tds_context : public NetImpl,
                         public detail::net_packet_xmit_context<tds_context<NetImpl>>,
                         public detail::net_packet_recv_context<tds_context<NetImpl>> {

        using tds_context_type = tds_context<NetImpl>;
        using xmit_context     = detail::net_packet_xmit_context<tds_context_type>;
        using recv_context     = detail::net_packet_recv_context<tds_context_type>;

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
        } TDSLITE_PACKED;

        using row_callback_fn_t = void (*)(void *, const tds_colmetadata_token &, const tdsl_row &);

    private:
        // ENVCHANGE token callback context
        callback_context<tds_envchange_token> envinfochg_cb_ctx{};
        // INFO/ERROR token callback context
        callback_context<tds_info_token> info_cb_ctx{};
        // LOGINACK token callback context
        callback_context<tds_login_ack_token> loginack_cb_ctx{};
        // DONE token callback context
        callback_context<tds_done_token> done_cb_ctx{};
        // COLMETADATA token callback context
        callback_context<void, tdsl::uint32_t (*)(void *, tds_colmetadata_token &)> colmetadata_cb_ctx{};
        // ROW callback context
        callback_context<void, row_callback_fn_t> row_cb_ctx{};

        struct {
            // Indicates that the tds_context is authenticated against
            // the connected server.
            bool authenticated : 1;
            // Column names should be read into memory.
            // By default, they're discarded for memory reasons.
            bool read_colnames : 1;
            bool reserved : 6;
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
            // If you are hitting this static assertion, it means either your NetImpl does not have register_msg_recv_callback,
            // or it does not have the expected function signature. The `register_msg_recv_callback` function is provided
            // by the `network_impl_base` type, which MUST be inherited by every single NetImpl. So, either the NetImpl
            // you have provided does not inherit from `network_impl_base`, or the inheritance is private. Go figure.
            static_assert(traits::dependent_bool<detail::has_register_msg_recv_member_fn<tds_context<NetImpl>>::value>::value,
                          "The type NetImpl must implement void register_msg_recv_callback(void*, tdsl::uint32_t (*)(void *, "
                          "tdsl::detail::e_tds_message_type, tdsl::span<const tdsl::uint8_t>)) function!");
            // Register tds_context message handler to network implementation
            this->register_msg_recv_callback(this, &handle_msg);
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
            this->template write(TDSLITE_OFFSETOF(tds_header, length),
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
        static tdsl::uint32_t handle_msg(void * self_optr, tdsl::detail::e_tds_message_type mt, tdsl::span<const tdsl::uint8_t> message) {
            using msg_type = tdsl::detail::e_tds_message_type;
            auto self      = reinterpret_cast<tds_context_type *>(self_optr);
            auto rr        = tdsl::binary_reader<tdsl::endian::little>{message};

            switch (mt) {
                case msg_type::tabular_result: {
                    return self->handle_tabular_result_msg(rr);
                } break;
                default: {
                    TDSLITE_DEBUG_PRINT("tds_context::handle_msg: unhandled (%d) bytes of msg with type (%d)", message.size_bytes(),
                                        static_cast<int>(mt));
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
        TDSLITE_SYMBOL_VISIBLE void do_register_envchange_token_callback(void * user_ptr,
                                                                         typename decltype(envinfochg_cb_ctx)::function_type cb) {
            envinfochg_cb_ctx.set(user_ptr, cb);
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
        TDSLITE_SYMBOL_VISIBLE void do_register_info_token_callback(void * user_ptr, typename decltype(info_cb_ctx)::function_type cb) {
            info_cb_ctx.set(user_ptr, cb);
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
        TDSLITE_SYMBOL_VISIBLE void do_register_loginack_token_callback(void * user_ptr,
                                                                        typename decltype(loginack_cb_ctx)::function_type cb) {
            loginack_cb_ctx.set(user_ptr, cb);
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
        TDSLITE_SYMBOL_VISIBLE void do_register_done_token_callback(void * user_ptr, typename decltype(done_cb_ctx)::function_type cb) {
            done_cb_ctx.set(user_ptr, cb);
        }

        // --------------------------------------------------------------------------------

        /**
         * Register COLMETADATA token callback.
         *
         * The given callback function will be invoked with token info
         * when a COLMETADTA token is received.
         *
         * The @p user_ptr will be passed as the first argument to callback
         * function
         *
         * @param [in] user_ptr User pointer
         * @param [in] cb Callback function
         */
        TDSLITE_SYMBOL_VISIBLE void do_register_colmetadata_token_callback(void * user_ptr,
                                                                           typename decltype(colmetadata_cb_ctx)::function_type cb) {
            colmetadata_cb_ctx.set(user_ptr, cb);
        }

        // --------------------------------------------------------------------------------

        /**
         * Register ROW token callback.
         *
         * The given callback function will be invoked with token info
         * when a ROW token is received.
         *
         * The @p user_ptr will be passed as the first argument to callback
         * function
         *
         * @param [in] user_ptr User pointer
         * @param [in] cb Callback function
         */
        TDSLITE_SYMBOL_VISIBLE void do_register_row_callback(void * user_ptr, typename decltype(row_cb_ctx)::function_type cb) {
            row_cb_ctx.set(user_ptr, cb);
        }

    private:
        struct colmetadata_read_result {
            enum class status
            {
                success                  = 0,
                not_enough_bytes         = -1,
                not_enough_memory        = -2,
                unknown_column_size_type = -3,
            };
            status status;
            tdsl::uint32_t need_bytes;
        };

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
        inline tdsl::uint32_t handle_tabular_result_msg(tdsl::binary_reader<tdsl::endian::little> & rr) noexcept {
            using token_type = tdsl::detail::e_tds_message_token_type;
            tds_colmetadata_token col_metadata{}; // column metadata token, for row results

            constexpr int k_min_token_need_bytes = 3;
            // start parsing tokens
            while (rr.has_bytes(/*amount_of_bytes=*/k_min_token_need_bytes)) {
                const auto tt                = static_cast<token_type>(rr.read<tdsl::uint8_t>());

                // TDSLITE_ASSERT_MSG( < length,
                //                    "Something is wrong, token length is greater than the length of the encapsulating TDS PDU!");

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
                tdsl::uint32_t subhandler_nb = {0};
                switch (tt) {
                    case token_type::colmetadata: {
                        auto result = read_colmetadata_token(rr, col_metadata);
                        if (result.status == decltype(result)::status::not_enough_bytes) {
                            subhandler_nb = result.need_bytes;
                            break;
                        }
                        switch (result.status) {
                            case decltype(result)::status::success:
                                break;
                            case decltype(result)::status::not_enough_memory: {
                                TDSLITE_DEBUG_PRINT("Unable to handle COLMETADATA token, not enough memory!\n");
                                // FIXME: invoke error callback?
                                return 0;
                            } break;
                            default: {
                                TDSLITE_DEBUG_PRINT("Unknown COLMETADATA handler status code %d, discarding packet\n",
                                                    static_cast<int>(result.status));
                                // FIXME: invoke error callback?
                                return 0;
                            }
                        }
                    } break;
                    case token_type::row: {
                        if (not col_metadata.is_valid()) {
                            // encountered row info without colmetadata?
                            TDSLITE_DEBUG_PRINT("Encountered ROW token withour prior COLMETADATA token, discarding packet\n");
                            // FIXME: invoke error callback?
                            return 0;
                        }
                        subhandler_nb = handle_row_token(rr, col_metadata);
                    } break;
                    default: {
                        tdsl::uint16_t current_token_size = 0;
                        // Possibility #2: Tokens with size information available
                        // beforehand. These are easier to parse since we know
                        // how many bytes to expect
                        if (not(tt == token_type::done)) {
                            current_token_size = rr.read<tdsl::uint16_t>();
                        }
                        else {
                            current_token_size = {8}; // sizeof done token(fixed)
                        }

                        // Ensure that we got enough bytes to read the token
                        if (not(rr.has_bytes(current_token_size))) {
                            return current_token_size - rr.remaining_bytes();
                        }

                        auto token_reader = rr.subreader(current_token_size);
                        switch (tt) {
                            case token_type::envchange: {
                                subhandler_nb = handle_envchange_token(token_reader);
                            } break;
                            case token_type::error:
                            case token_type::info: {
                                subhandler_nb = handle_info_token(token_reader);
                            } break;
                            case token_type::done: {
                                subhandler_nb = handle_done_token(token_reader);
                            } break;
                            case token_type::loginack: {
                                subhandler_nb = handle_loginack_token(rr);
                            } break;

                            default: {
                                TDSLITE_DEBUG_PRINT("Unhandled TOKEN type [%d (%s)]\n", static_cast<int>(tt),
                                                    message_token_type_to_str(static_cast<token_type>(tt)));
                            } break;
                        }
                        // Advance to the next token
                        rr.advance(current_token_size);
                    }
                }

                // If we need more bytes to proceed, ask for more bytes
                if (subhandler_nb > 0) {
                    return subhandler_nb;
                }
            }

            // "All integer types are represented in reverse byte order (little-endian) unless otherwise specified."

            TDSLITE_ASSERT_MSG(rr.remaining_bytes() < 3,
                               "There are more than 3 bytes remaining in the reader, something is wrong in token handler loop!");

            // We expect here to have consumed all of the reader. If that is not the case
            // that means we got partial data.
            return (rr.remaining_bytes() == 0 ? 0 : k_min_token_need_bytes - rr.remaining_bytes());
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
                    TDSLITE_RETIF_LESS_BYTES(rr, 1);

                    // Read new value
                    const auto nvlen_octets = (rr.read<tdsl::uint8_t>() * 2);
                    TDSLITE_RETIF_LESS_BYTES(rr, nvlen_octets + 1); // +1 for ovlen
                    const auto nvval        = rr.read(nvlen_octets);
                    // Read old value
                    const auto ovlen_octets = (rr.read<tdsl::uint8_t>() * 2);
                    TDSLITE_RETIF_LESS_BYTES(rr, ovlen_octets);
                    const auto ovval = rr.read(ovlen_octets);

                    tds_envchange_token envchange_info{};
                    envchange_info.type      = ect;
                    envchange_info.new_value = nvval.rebind_cast<char16_t>();
                    envchange_info.old_value = ovval.rebind_cast<char16_t>();
                    TDSLITE_ASSERT_MSG(rr.remaining_bytes() == 0,
                                       "There are unhandled stray bytes in ENVCHANGE token, probably something is wrong!");
                    TDSLITE_DEBUG_PRINT("received environment change -> type [%d] | ", static_cast<int>(envchange_info.type));
                    TDSLITE_DEBUG_PRINT("new_value: [");
                    TDSLITE_DEBUG_PRINT_U16_AS_MB(envchange_info.new_value);
                    TDSLITE_DEBUG_PRINT("] | ");
                    TDSLITE_DEBUG_PRINT("old_value: [");
                    TDSLITE_DEBUG_PRINT_U16_AS_MB(envchange_info.old_value);
                    TDSLITE_DEBUG_PRINT("]\n");
                    return envinfochg_cb_ctx.maybe_invoke(envchange_info);
                } break;
                default: {
                    TDSLITE_DEBUG_PRINT("Unhandled ENVCHANGE type [%d]\n", static_cast<int>(ect));
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

            TDSLITE_TRY_READ_U16_VARCHAR(msgtext, rr);
            TDSLITE_TRY_READ_U8_VARCHAR(server_name, rr);
            TDSLITE_TRY_READ_U8_VARCHAR(proc_name, rr);

            info_msg.line_number = rr.read<tdsl::uint16_t>();

            info_msg.msgtext     = msgtext.rebind_cast<char16_t>();
            info_msg.server_name = server_name.rebind_cast<char16_t>();
            info_msg.proc_name   = proc_name.rebind_cast<char16_t>();

            TDSLITE_ASSERT_MSG(rr.remaining_bytes() == 0, "There are unhandled stray bytes in INFO token, probably something is wrong!");

            TDSLITE_DEBUG_PRINT("received info message -> number [%d] | state [%d] | class [%d] | line number [%d] | ", info_msg.number,
                                info_msg.state, info_msg.class_, info_msg.line_number);
            TDSLITE_DEBUG_PRINT("msgtext: [");
            TDSLITE_DEBUG_PRINT_U16_AS_MB(info_msg.msgtext);
            TDSLITE_DEBUG_PRINT("] | ");
            TDSLITE_DEBUG_PRINT("server_name: [");
            TDSLITE_DEBUG_PRINT_U16_AS_MB(info_msg.server_name);
            TDSLITE_DEBUG_PRINT("] | ");
            TDSLITE_DEBUG_PRINT("proc_name: [");
            TDSLITE_DEBUG_PRINT_U16_AS_MB(info_msg.proc_name);
            TDSLITE_DEBUG_PRINT("]\n");

            return info_cb_ctx.maybe_invoke(info_msg);
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
            TDSLITE_TRY_READ_U8_VARCHAR(progname, rr);

            if (not rr.has_bytes(k_progver_bytes)) {
                return k_progver_bytes - rr.remaining_bytes();
            }

            token.prog_version.maj         = rr.read<tdsl::uint8_t>();
            token.prog_version.min         = rr.read<tdsl::uint8_t>();
            token.prog_version.buildnum_hi = rr.read<tdsl::uint8_t>();
            token.prog_version.buildnum_lo = rr.read<tdsl::uint8_t>();
            token.prog_name                = progname.rebind_cast<char16_t>();

            TDSLITE_DEBUG_PRINT("received login ack token -> interface [%d] | tds version [0x%x] | ", +token.interface, token.tds_version);
            TDSLITE_DEBUG_PRINT("prog_name: [");
            TDSLITE_DEBUG_PRINT_U16_AS_MB(token.prog_name);
            TDSLITE_DEBUG_PRINT("] | ");
            TDSLITE_DEBUG_PRINT("prog_version: [%d.%d.%d.%d]\n", token.prog_version.maj, token.prog_version.min,
                                token.prog_version.buildnum_hi, token.prog_version.buildnum_lo);
            loginack_cb_ctx.maybe_invoke(token);
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

            TDSLITE_DEBUG_PRINT("received done token -> status [%d] | cur_cmd [%d] | done_row_count [%d]\n", token.status, token.curcmd,
                                token.done_row_count);
            done_cb_ctx.maybe_invoke(token);
            return 0;
        }

        // --------------------------------------------------------------------------------

        /**
         * Handler for COLMETADATA token type
         *
         * The function parses the given data in @p rr as COLMETADATA token and calls the info
         * callback function, if a callback function is assigned.
         *
         * @param [in] rr Reader to read from
         * @param [out] out Variable to put parsed COLMETADATA token data
         *
         * @return tdsl::uint32_t Amount of needed bytes to read a complete COLMETADATA token, if any.
         *                        The return value would be non-zero only if the reader has partial
         *                        token data.
         */
        inline colmetadata_read_result read_colmetadata_token(tdsl::binary_reader<tdsl::endian::little> & rr, tds_colmetadata_token & out) {
            colmetadata_read_result result{};

            constexpr static auto k_min_colmetadata_bytes = 8;
            if (not rr.has_bytes(k_min_colmetadata_bytes)) {
                result.status     = decltype(result)::status::not_enough_bytes;
                result.need_bytes = k_min_colmetadata_bytes - rr.remaining_bytes();
                TDSLITE_DEBUG_PRINT("received COLMETADATA token, not enough bytes need (at least) %d, have %ld\n", k_min_colmetadata_bytes,
                                    rr.remaining_bytes());
                return result;
            }

            auto column_count = rr.read<tdsl::uint16_t>();
            if (not out.allocate_colinfo_array(column_count)) {
                result.status = decltype(result)::status::not_enough_memory;
                TDSLITE_DEBUG_PRINT("failed to allocate memory for column info for %d column(s)", column_count);
                return result;
            }

            if (flags.read_colnames) {
                // Allocate column name array
                if (not out.allocate_column_name_array(column_count)) {
                    result.status = decltype(result)::status::not_enough_memory;
                    TDSLITE_DEBUG_PRINT("failed to allocate memory for column name array for %d column(s)", column_count);
                    return result;
                }
            }

            constexpr int k_colinfo_min_bytes = 6; // user_type + flags + type + colname len
            auto colindex                     = 0;
            while (colindex < column_count && rr.has_bytes(k_colinfo_min_bytes)) {
                tds_column_info & current_column = out.columns [colindex];

                // keep column names separate? makes sense. allocate only if user decided to see column names.
                current_column.user_type         = rr.read<tdsl::uint16_t>();
                current_column.flags             = rr.read<tdsl::uint16_t>();
                current_column.type              = static_cast<e_tds_data_type>(rr.read<tdsl::uint8_t>());

                const auto dtype_props           = get_data_type_props(current_column.type);

                if (not rr.has_bytes(/*amount_of_bytes=*/dtype_props.length.variable.length_size + 1)) {
                    result.status     = decltype(result)::status::not_enough_bytes;
                    result.need_bytes = (dtype_props.length.variable.length_size + 1) - rr.remaining_bytes();
                    return result;
                }

                // COLLATION occurs only if the type is BIGCHARTYPE, BIGVARCHARTYPE, TEXTTYPE, NTEXTTYPE,
                // NCHARTYPE, or NVARCHARTYPE.

                switch (dtype_props.size_type) {
                    case e_tds_data_size_type::fixed:
                        // No extra info to read for these
                        // types. Their length is fixed.
                        break;
                    case e_tds_data_size_type::var_u8:
                        // Variable length data types with 8-bit length bit width
                        current_column.typeprops.u8l.length = rr.read<tdsl::uint8_t>();
                        break;
                    case e_tds_data_size_type::var_u16:
                        // Variable length data types with 16-bit length bit width
                        current_column.typeprops.u16l.length = rr.read<tdsl::uint16_t>();
                        break;
                    case e_tds_data_size_type::var_u32:
                        // Variable length data types with 32-bit length bit width
                        current_column.typeprops.u32l.length = rr.read<tdsl::uint32_t>();
                        break;
                    case e_tds_data_size_type::var_precision:
                        // Types like DECIMALNTYPE & NUMERICNTYPE have precision
                        // and scale values. Precision determines the field's length
                        // whereas scale is the multiplier.
                        current_column.typeprops.ps.precision = rr.read<tdsl::uint8_t>();
                        current_column.typeprops.ps.scale     = rr.read<tdsl::uint8_t>();
                        break;
                    case e_tds_data_size_type::unknown:

                        TDSLITE_DEBUG_PRINT("unable to determine data type size for type %d, aborting read",
                                            static_cast<tdsl::uint8_t>(current_column.type));
                        result.status     = decltype(result)::status::unknown_column_size_type;
                        result.need_bytes = 0;
                        return result;
                }

                current_column.colname_length_in_chars = rr.read<tdsl::uint8_t>();
                const auto colname_len_in_bytes        = (current_column.colname_length_in_chars * 2);
                if (not rr.has_bytes((colname_len_in_bytes * 2))) {
                    result.status     = decltype(result)::status::not_enough_bytes;
                    result.need_bytes = colname_len_in_bytes - rr.remaining_bytes();
                    TDSLITE_DEBUG_PRINT("not enough bytes to read column name, need %d, have %ld", colname_len_in_bytes,
                                        rr.remaining_bytes());
                    return result;
                }

                if (not flags.read_colnames) {
                    TDSLITE_EXPECT(rr.advance(colname_len_in_bytes));
                }
                else {
                    const auto span = rr.read(colname_len_in_bytes);
                    if (not out.set_column_name(colindex, span)) {
                        result.status = decltype(result)::status::not_enough_memory;
                        TDSLITE_DEBUG_PRINT("failed to allocate memory for column index %d 's name", colindex);
                        return result;
                    }
                }
                ++colindex;
            }

            TDSLITE_DEBUG_PRINT("received COLMETADATA token -> column count [%d]\n", out.column_count);
            result.status     = decltype(result)::status::success;
            result.need_bytes = 0;
            return result;
        }

        // --------------------------------------------------------------------------------

        /**
         * Handler for ROW token type
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
        inline tdsl::uint32_t handle_row_token(tdsl::binary_reader<tdsl::endian::little> & rr, const tds_colmetadata_token & colmetadata) {
            // using data_type                        = tdsl::detail::e_tds_data_type;
            using data_size_type = tdsl::detail::e_tds_data_size_type;

            // constexpr static auto k_min_done_bytes = 8;

            // if (not rr.has_bytes(k_min_done_bytes)) {
            //     return k_min_done_bytes - rr.remaining_bytes();
            // }

            auto row_data{tdsl_row::make(colmetadata.column_count)};

            if (not row_data) {
                TDSLITE_DEBUG_PRINT("row data creation failed (%d)", static_cast<int>(row_data.error()));
                // report error
                return 0;
            }

            // Each row should contain N fields.
            for (tdsl::uint32_t cidx = 0; cidx < colmetadata.column_count; cidx++) {
                // TDSLITE_ASSERT_MSG(colmetadata.columns [cidx] != nullptr, "column info is null!");
                auto & field                = row_data->fields.data() [cidx];
                const auto & column         = colmetadata.columns [cidx];
                const auto & dprop          = get_data_type_props(column.type);

                tdsl::uint32_t field_length = 0;
                switch (dprop.size_type) {
                    case data_size_type::fixed:
                        field_length = dprop.length.fixed;
                        break;
                    case data_size_type::var_u8:
                        if (not rr.has_bytes(sizeof(tdsl::uint8_t))) {
                            return 1;
                        }
                        field_length = rr.read<tdsl::uint8_t>();
                        break;
                    case data_size_type::var_u16:
                        if (not rr.has_bytes(sizeof(tdsl::uint16_t))) {
                            return sizeof(tdsl::uint16_t);
                        }
                        field_length = rr.read<tdsl::uint16_t>();
                        break;
                    case data_size_type::var_u32:
                        if (not rr.has_bytes(sizeof(tdsl::uint32_t))) {
                            return sizeof(tdsl::uint32_t);
                        }
                        field_length = rr.read<tdsl::uint32_t>();
                        break;
                    case data_size_type::var_precision:
                        break;
                    case data_size_type::unknown:
                        TDSLITE_ASSERT_MSG(0, "unknown size_type");
                        TDSLITE_UNREACHABLE;
                        break;
                }

                if (not rr.has_bytes(field_length)) {
                    TDSLITE_DEBUG_PRINT("not enough bytes for reading field, %lu more bytes needed\n", field_length - rr.remaining_bytes());
                    return field_length - rr.remaining_bytes();
                }

                if (field_length) {
                    field.data = rr.read(field_length);
                }

                // (mgilor) `%.*s` does not work here, printf stops printing
                // characters immediately when it encounters a \0, regardless
                // of the provided string's length.

                TDSLITE_DEBUG_PRINT("row field %u -> [", cidx);
                TDSLITE_DEBUG_HEXPRINT(field.data.data(), field_length);
                TDSLITE_DEBUG_PRINT("]\n");
            }

            // Invoke row callback
            row_cb_ctx.maybe_invoke(colmetadata, row_data.get());

            return 0;
        }

        friend struct detail::net_packet_xmit_context<tds_context<NetImpl>>;
        friend struct detail::net_packet_recv_context<tds_context<NetImpl>>;
        friend struct tdsl::detail::login_context<NetImpl>;
    };
}} // namespace tdsl::detail