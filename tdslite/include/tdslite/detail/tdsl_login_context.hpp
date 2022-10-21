/**
 * _________________________________________________
 * Tabular Data Stream protocol login operations
 *
 * @file   tdsl_login_context.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   14.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSL_DETAIL_LOGIN_CONTEXT_HPP
#define TDSL_DETAIL_LOGIN_CONTEXT_HPP

#include <tdslite/detail/tdsl_lang_code_id.hpp>
#include <tdslite/detail/tdsl_version.hpp>
#include <tdslite/detail/tdsl_tds_context.hpp>
#include <tdslite/detail/tdsl_mssql_error_codes.hpp>
#include <tdslite/detail/tdsl_callback.hpp>
#include <tdslite/detail/tdsl_string_writer.hpp>
#include <tdslite/detail/tdsl_tds_header.hpp>

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_string_view.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_byte_swap.hpp>
#include <tdslite/util/tdsl_debug_print.hpp>

namespace tdsl { namespace detail {

    // --------------------------------------------------------------------------------

    /**
     * @ref login_context login helper class
     *
     * This class prepares a login packet and performs login operation
     * over a network connection established to the database engine.
     *
     * @tparam NetImpl Network backend implementation to use
     *
     * (e.g. net_arduino_uipethernet, net_arduino_ethernet, net_boost_asio)
     */
    template <typename NetImpl>
    struct login_context {
        using tds_context_type = tds_context<NetImpl>;
        using self_type        = login_context<NetImpl>;

        /**
         * The tabular data stream protocol header
         */
        struct tds_login7_header {
            tdsl::uint32_t packet_length;
            tdsl::uint32_t tds_version;
            tdsl::uint32_t packet_size;
            tdsl::uint32_t client_version;
            tdsl::uint32_t client_pid;
            tdsl::uint32_t connection_id;
            tdsl::uint8_t opt1;
            tdsl::uint8_t opt2;
            tdsl::uint8_t type;
            tdsl::uint8_t opt3;
            tdsl::uint32_t time_zone;
            tdsl::uint32_t collation;
        } TDSL_PACKED;

        static_assert(sizeof(tds_login7_header) == 36, "Invalid TDS Login7 header size");

        enum class e_login_status : tdsl::int8_t
        {
            success = 0,
            failure = -1
        };

    private:
        tds_context_type & tds_ctx;

    public:
        /**
         * Construct a new login context object
         *
         * @param [in] tc The TDS context
         */
        inline login_context(tds_context_type & tc) noexcept : tds_ctx(tc) {
            tds_ctx.callbacks.loginack = {
                this, +[](void * uptr, const tds_login_ack_token &) noexcept -> void {
                    auto self                         = reinterpret_cast<self_type *>(uptr);
                    // Mark current context as `authenticated`
                    self->tds_ctx.flags.authenticated = {true};
                }};
        }

        ~login_context() = default;

        /**
         * The arguments for the login operation
         *
         * @tparam StringViewType View type for string variables (default: string_view)
         */
        template <typename StringViewType = string_view>
        struct login_parameters_type {
            using sv_type = StringViewType;
            // Target server hostname or IP address
            // Length: At most 128 characters
            StringViewType server_name;
            StringViewType db_name;                 // Target database name
            StringViewType user_name;               // Database user name
            StringViewType password;                // Database user password
            StringViewType app_name;                // Client application name
            StringViewType client_name{""};         // The client machine name
            StringViewType library_name{"tdslite"}; // The library name
            tdsl::uint32_t packet_size{4096};       // Desired packet size (default = 4096)
            tdsl::uint32_t client_program_version{
                native_to_be(/*v=*/0x0BADC0DE_tdsu32)}; // Client program version
            tdsl::uint32_t client_pid{0};               // Client PID
            tdsl::uint32_t connection_id{0};            // Connection ID
            tdsl::uint8_t option_flags_1{0xE0};         // Option Flags (1)
            tdsl::uint8_t option_flags_2{0x03};         // Option Flags (2)
            tdsl::uint8_t sql_type_flags{0x00};         // SQL type flags
            tdsl::uint8_t option_flags_3{0x00};         // Option Flags (3)
            tdsl::uint32_t timezone{0};                 // Timezone
            tdsl::uint32_t collation{0};                // Collation
            tdsl::uint8_t client_id [6]{0};             // Client ID
        };

        /**
         * Login parameters (single-character)
         */
        using login_parameters  = login_parameters_type<string_view>;

        /**
         * Login parameters (wide-character)
         */
        using wlogin_parameters = login_parameters_type<wstring_view>;

        enum class e_tds_login_parameter_idx : tdsl::uint8_t
        {
            begin         = 0,
            client_name   = begin,
            user_name     = 1,
            password      = 2,
            app_name      = 3,
            server_name   = 4,
            unused        = 5,
            library_name  = 6,
            locale        = 7,
            database_name = 8,
            client_id     = 9,
            sspi          = 10,
            atchdbfile    = 11,
            end
        };

        // 14 bytes? 8 = 4

        constexpr static tdsl::uint16_t calc_sizeof_offset_size_section() noexcept {
            return ((static_cast<tdsl::uint16_t>(e_tds_login_parameter_idx::end) - 1) *
                    sizeof(tdsl::uint32_t)) +
                   6 /*client id size*/;
        }

        /**
         * Encode a password for TDS protocol
         *
         * @param [in] buf Buffer to encode
         */
        static inline void encode_password(tdsl::uint8_t * buf, tdsl::uint32_t sz) {
            // Quoting TDS:
            // "Before submitting a password from the client to the server, for every byte in the
            // password buffer" "starting with the position pointed to by ibPassword or
            // ibChangePassword, the client SHOULD first swap" "the four high bits with the four low
            // bits and then do a bit-XOR with 0xA5 (10100101).""
            for (tdsl::uint32_t i = 0; i < sz; i++) {
                auto & ch = buf [i];
                ch        = (((ch & 0x0F) << 4 | (ch & 0xF0) >> 4) ^ 0xA5);
            }
        }

        // --------------------------------------------------------------------------------

        /**
         * Attempt to login into the database engine with the specified login parameters
         *
         * @note Parameters on this overload are single-byte character strings.
         *
         * @param [in] params Login parameters
         */
        inline auto do_login(const wlogin_parameters & params) noexcept -> e_login_status {
            return do_login_impl<wlogin_parameters>(params);
        }

        // --------------------------------------------------------------------------------

        /**
         * Attempt to login into the database engine with the specified login parameters
         *
         * @note Parameters on this overload are single-byte character strings.
         *
         * @param [in] params Login parameters
         */
        inline auto do_login(const login_parameters & params) noexcept -> e_login_status {
            return do_login_impl<login_parameters>(params);
        }

    private:
        inline auto put_login_header_length(tdsl::uint32_t lplength) noexcept -> void {
            tds_ctx.write_le(TDSL_OFFSETOF(tds_login7_header, packet_length), lplength);
        }

        /**
         * Attempt to login into the database engine with the specified login parameters
         *
         * @note Parameters on this overload are single-byte character strings.
         *
         * @param [in] params Login parameters
         */
        template <typename LoginParamsType>
        e_login_status do_login_impl(const LoginParamsType & params) noexcept {
            tds_ctx.write_le(/*arg=*/0_tdsu32); // placeholder for packet length
            tds_ctx.write_be(
                static_cast<tdsl::uint32_t>(e_tds_version::sql_server_2000_sp1)); // TDS version
            tds_ctx.write_le(params.packet_size);            // Requested packet size by the client
            tds_ctx.write_le(params.client_program_version); // Client program version
            tds_ctx.write_le(params.client_pid);             // Client program PID
            tds_ctx.write_le(params.connection_id);          // Connection ID
            tds_ctx.write(params.option_flags_1);            // Option Flags (1)
            tds_ctx.write(params.option_flags_2);            // Option Flags (2)
            tds_ctx.write(params.sql_type_flags);            // Type Flags
            tds_ctx.write(params.option_flags_3);            // Option Flags (3)
            tds_ctx.write_le(params.timezone);               // Client Timezone (unused)
            tds_ctx.write_le(params.collation);              // Client language code ID

            // Calculate the total packet data section size.
            tdsl::uint16_t total_packet_data_size =
                (sizeof(tds_login7_header)) + calc_sizeof_offset_size_section();
            // Calculate the starting positiof of the offset/size table. This will act as base
            // offset for the offset/size table's offset values.
            constexpr tdsl::uint32_t string_table_offset_start =
                (sizeof(tds_login7_header)) + calc_sizeof_offset_size_section();

            // Indicates the offset of the current string in the string table
            tdsl::uint16_t current_string_offset = string_table_offset_start;

            static_assert(
                tdsl::traits::is_same<decltype(current_string_offset), tdsl::uint16_t>::value,
                "The `current_string_offset` variable is intended to be an uint16!");

            // Length of the string section in the data packet (sum of all string lengths)
            tdsl::uint16_t string_section_size = 0;

            // We will fill the offset/length table and string table in two passes:
            //  - Pass 0: Fill the offset/length table
            //  - Pass 1: Fill the strings
            enum class pass_type : tdsl::uint8_t
            {
                begin             = 0,
                offset_size_table = begin,
                string_table,
                end
            };

            using param_idx = e_tds_login_parameter_idx;

            for (pass_type pt = pass_type::begin; pt < pass_type::end;
                 pt           = static_cast<pass_type>(static_cast<tdsl::uint8_t>(pt) + 1)) {
                for (param_idx pi = param_idx::begin; pi < param_idx::end;
                     pi           = static_cast<param_idx>(static_cast<tdsl::uint8_t>(pi) + 1)) {
                    // Write offset and length values for each extension
                    const typename LoginParamsType::sv_type * tw = {nullptr};
                    switch (pi) {
                        case param_idx::client_name:
                            tw = &params.client_name;
                            break;
                        case param_idx::user_name:
                            tw = &params.user_name;
                            break;
                        case param_idx::password:
                            tw = &params.password;
                            break;
                        case param_idx::app_name:
                            tw = &params.app_name;
                            break;
                        case param_idx::server_name:
                            tw = &params.server_name;
                            break;
                        case param_idx::database_name:
                            tw = &params.db_name;
                            break;
                        case param_idx::library_name:
                            tw = &params.library_name;
                            break;
                        case param_idx::client_id:
                            if (pt == pass_type::offset_size_table) {
                                tds_ctx.write(
                                    tdsl::span<>{&params.client_id [0], sizeof(params.client_id)});
                            }
                            continue;
                            break;
                        case param_idx::unused:
                        case param_idx::sspi:
                            if (pt == pass_type::offset_size_table) {
                                tds_ctx.write(/*arg=*/0_tdsu32);
                            }
                            continue;
                            break;
                        case param_idx::locale:
                        case param_idx::atchdbfile:

                            if (pt == pass_type::offset_size_table) {
                                // offset(u16), size(u16)
                                tds_ctx.write(current_string_offset);
                                tds_ctx.write(/*arg=*/0_tdsu16);
                            }
                            continue;
                        default: {
                            TDSL_ASSERT(false);
                            TDSL_UNREACHABLE;
                        } break;
                    } // ... switch (static_cast<e_tds_login_parameter_idx>(j))

                    TDSL_ASSERT(tw);

                    if (pt == pass_type::offset_size_table) {
                        // We're filling the offset table (first pass)
                        tds_ctx.write_le(current_string_offset);
                        // Character count, not length in bytes
                        tds_ctx.write_le(static_cast<tdsl::uint16_t>((*tw).size()));

                        current_string_offset +=
                            string_parameter_writer<tds_context_type>::calculate_write_size((*tw));
                    }
                    else {

                        // Client ID is offset section only.
                        if (pi == param_idx::client_id) {
                            continue;
                        }

                        // Write the string itself.
                        if (*tw) {
                            string_parameter_writer<tds_context_type>::write(
                                tds_ctx, *tw,
                                (pi == param_idx::password ? &encode_password : nullptr));
                            string_section_size +=
                                string_parameter_writer<tds_context_type>::calculate_write_size(
                                    (*tw));
                        }
                    }
                } // ... for (int j = 0; j < parameter_count; j++) {
            }     // ... for (int i = 0; i < 2; i++)

            // We've already added the offset/size section's
            // size, so only add the string section's size and
            // the client id size here.
            total_packet_data_size += string_section_size;

            // Now we exactly know how much packet data we got our hand.
            // Replace the placeholder value (0) with the actual packet
            // length.
            // Write the login packet length
            put_login_header_length(total_packet_data_size);
            // Send the login request.
            tds_ctx.send_tds_pdu(e_tds_message_type::login);

            // Receive the login response
            tds_ctx.receive_tds_pdu();

            return tds_ctx.is_authenticated() ? e_login_status::success : e_login_status::failure;
        } // ... void do_login_impl(const LoginParamsType & params) noexcept {
    };
}} // namespace tdsl::detail

#endif