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

#pragma once

#include <tdslite/detail/tdsl_lang_code_id.hpp>
#include <tdslite/detail/tdsl_version.hpp>
#include <tdslite/detail/tdsl_tds_context.hpp>

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_byte_swap.hpp>

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

        /**
         * Construct a new login context object
         *
         * @param [in] tc The TDS context
         */
        inline login_context(tds_context_type & tc) noexcept : tds_ctx(tc) {
            tds_ctx.set_receive_callback(this, &handle_response);
        }

        /**
         * The tabular data stream protocol header
         */
        struct tds_login7_header {
            typename tds_context_type::tds_header th;
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
        } TDSLITE_PACKED;

        static_assert(sizeof(tds_login7_header) == 44, "Invalid TDS Login7 header size");

        /**
         * Wide-character (16 bit) string view
         */
        struct wstring_view : public tdsl::span<const char16_t> {

            using ::tdsl::span<const char16_t>::span;

            wstring_view() : span() {}

            template <tdsl::uint32_t N>
            wstring_view(const char (&str) [N]) : span(str, N) {
                // If the string is NUL-terminated, omit the NUL terminator.
                if (N > 1 && str [N - 2] == '\0' && str [N - 1] == '\0') {
                    size_ -= 2;
                }
            }
        };

        /**
         * String view
         */
        struct string_view : public tdsl::span<const char> {
            using ::tdsl::span<const char>::span;

            string_view() : span() {}

            template <tdsl::uint32_t N>
            string_view(const char (&str) [N]) : span(str, N) {
                // If the string is NUL-terminated, omit the NUL terminator.
                if (N > 0 && str [N - 1] == '\0') {
                    size_ -= 1;
                }
            }
        };

        enum class e_login_status : tdsl::int8_t
        {
            success = 0,
            failure = -1
        };

        using login_callback_type = void (*)(e_login_status);

        /**
         * The arguments for the login operation
         *
         * @tparam StringViewType View type for string variables (default: string_view)
         */
        template <typename StringViewType = string_view>
        struct login_parameters_type {
            using sv_type = StringViewType;
            StringViewType server_name;                    // Target server hostname or IP address
            StringViewType db_name;                        // Target database name
            StringViewType user_name;                      // Database user name
            StringViewType password;                       // Database user password
            StringViewType app_name;                       // Client application name
            StringViewType client_name{""};                // The client machine name
            StringViewType library_name{"tdslite"};        // The library name
            tdsl::uint32_t packet_size{4096};              // Desired packet size (default = 4096)
            tdsl::uint32_t client_program_version{101010}; // Client program version
            tdsl::uint32_t client_pid{0};                  // Client PID
            tdsl::uint32_t connection_id{0};               // Connection ID
            tdsl::uint8_t option_flags_1{0xE0};            // Option Flags (1)
            tdsl::uint8_t option_flags_2{0x03};            // Option Flags (2)
            tdsl::uint8_t sql_type_flags{0x00};            // SQL type flags
            tdsl::uint8_t option_flags_3{0x00};            // Option Flags (3)
            tdsl::uint32_t timezone{0};                    // Timezone
            tdsl::uint32_t collation{0};                   // Collation
            tdsl::uint8_t client_id [6]{0};                // Client ID
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
            return ((static_cast<tdsl::uint16_t>(e_tds_login_parameter_idx::end) - 1) * sizeof(tdsl::uint32_t)) + 6 /*client id size*/;
        }

        /**
         * Encode a password for TDS protocol
         *
         * @param [in] buf Buffer to encode
         */
        template <tdsl::uint32_t N>
        static inline void encode_password(tdsl::uint8_t (&buf) [N]) noexcept {
            // Quoting TDS:
            // "Before submitting a password from the client to the server, for every byte in the password buffer"
            // "starting with the position pointed to by ibPassword or ibChangePassword, the client SHOULD first
            // swap" "the four high bits with the four low bits and then do a bit-XOR with 0xA5 (10100101).""
            for (auto & ch : buf) {
                ch = (((ch & 0x0F) << 4 | (ch & 0xF0) >> 4) ^ 0xA5);
            }
        }

        // --------------------------------------------------------------------------------

        /**
         * Helper type for writing string parameters into a TDS packet.
         *
         * The string parameters are handled depending on source string type
         * (i.e. single-char string, wide (utf-16) string)).
         *
         */
        struct string_parameter_writer {
            /**
             * Write single-byte string in @p sv to transmit context @p xc as multi-byte
             * UTF-16 string.
             *
             * @param [in,out] xc Transmit context
             * @param [in] sv String to write
             * @param [in] parameter_type Type of the string parameter.
             *
             * @note The password string also get encoded by the algorithm specified in the MS-TDS document
             * @note The password encoding is performed after multi-byte string expansion
             */
            static void write(typename tds_context_type::xmit_context & xc, const string_view & sv,
                              e_tds_login_parameter_idx parameter_type) {
                // We're filling the strings
                for (auto ch : sv) {
                    char16_t c = ch;
                    e_tds_login_parameter_idx::password == parameter_type
                        ? encode_password(reinterpret_cast<tdsl::uint8_t(&) [sizeof(char16_t)]>(c))
                        : (void) 0;

                    xc.write(c);
                }
            }

            /**
             * Write multi-byte string in @p sv to transmit context @p xc as-is.
             *
             * @param [in] xc Transmit context
             * @param [in] sv String to write
             * @param [in] parameter_type Type of the string parameter
             *
             * @note The password string also get encoded by the algorithm specified in the MS-TDS document
             */
            static void write(typename tds_context_type::xmit_context & xc, const wstring_view & sv,
                              e_tds_login_parameter_idx parameter_type) {

                if (not(e_tds_login_parameter_idx::password == parameter_type)) {
                    xc.write(sv);
                    return;
                }

                // Password needs special treatment before sending.
                for (auto ch : sv) {
                    char16_t c = ch;
                    encode_password(reinterpret_cast<tdsl::uint8_t(&) [sizeof(char16_t)]>(ch));
                    xc.write(c);
                }
            }

            static inline auto calculate_write_size(const wstring_view & sv) noexcept -> tdsl::uint32_t {
                return sv.size_bytes();
            }

            static inline auto calculate_write_size(const string_view & sv) noexcept -> tdsl::uint32_t {
                return sv.size_bytes() * sizeof(char16_t);
            }
        };

        // --------------------------------------------------------------------------------

        /**
         * Attempt to login into the database engine with the specified login parameters
         *
         * @note Parameters on this overload are single-byte character strings.
         *
         * @param [in] params Login parameters
         */
        inline void do_login(const wlogin_parameters & params, login_callback_type lcb) noexcept {
            do_login_impl<wlogin_parameters>(params, lcb);
        }

        // --------------------------------------------------------------------------------

        /**
         * Attempt to login into the database engine with the specified login parameters
         *
         * @note Parameters on this overload are single-byte character strings.
         *
         * @param [in] params Login parameters
         */
        inline void do_login(const login_parameters & params, login_callback_type lcb) noexcept {
            do_login_impl<login_parameters>(params, lcb);
        }

    private:
        inline auto put_login_header_length(tdsl::uint32_t lplength) noexcept -> void {
            tds_ctx.write_le(TDSLITE_OFFSETOF(tds_login7_header, packet_length), lplength);
        }
        /**
         * Attempt to login into the database engine with the specified login parameters
         *
         * @note Parameters on this overload are single-byte character strings.
         *
         * @param [in] params Login parameters
         */
        template <typename LoginParamsType>
        void do_login_impl(const LoginParamsType & params, login_callback_type lcb) noexcept {
            // Assign login callback first
            login_callback = lcb;
            tds_ctx.write_tds_header(e_tds_message_type::login);
            tds_ctx.write_le(0_tdsu32);                                                        // placeholder for packet length
            tds_ctx.write_be(static_cast<tdsl::uint32_t>(e_tds_version::sql_server_2000_sp1)); // TDS version
            tds_ctx.write_le(params.packet_size);                                              // Requested packet size by the client
            tds_ctx.write_le(params.client_program_version);                                   // Client program version
            tds_ctx.write_le(params.client_pid);                                               // Client program PID
            tds_ctx.write_le(params.connection_id);                                            // Connection ID
            tds_ctx.write(params.option_flags_1);                                              // Option Flags (1)
            tds_ctx.write(params.option_flags_2);                                              // Option Flags (2)
            tds_ctx.write(params.sql_type_flags);                                              // Type Flags
            tds_ctx.write(params.option_flags_3);                                              // Option Flags (3)
            tds_ctx.write_le(params.timezone);                                                 // Client Timezone (unused)
            tds_ctx.write_le(params.collation);                                                // Client language code ID

            // Calculate the total packet data section size.
            tdsl::uint16_t total_packet_data_size =
                (sizeof(tds_login7_header) - sizeof(typename tds_context_type::tds_header)) + calc_sizeof_offset_size_section();
            // Calculate the starting positiof of the offset/size table. This will act as base
            // offset for the offset/size table's offset values.
            constexpr tdsl::uint32_t string_table_offset_start =
                (sizeof(tds_login7_header) - sizeof(typename tds_context_type::tds_header)) + calc_sizeof_offset_size_section();

            // Indicates the offset of the current string in the string table
            tdsl::uint16_t current_string_offset = string_table_offset_start;

            static_assert(tdsl::traits::is_same<decltype(current_string_offset), tdsl::uint16_t>::value,
                          "The `current_string_offset` variable is intended to be an uint16!");

            // Length of the string section in the data packet (sum of all string lengths)
            tdsl::uint16_t string_section_size = 0;

            // We will fill the offset/length table and string table in two passes:
            // Pass 0: Fill the offset/length table
            // Pass 1: Fill the strings
            enum class pass_type : tdsl::uint8_t
            {
                begin             = 0,
                offset_size_table = begin,
                string_table,
                end
            };
            using param_idx = e_tds_login_parameter_idx;

            for (pass_type pt = pass_type::begin; pt < pass_type::end; pt = static_cast<pass_type>(static_cast<tdsl::uint8_t>(pt) + 1)) {
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
                                tds_ctx.write(tdsl::span<>{&params.client_id [0], sizeof(params.client_id)});
                            }
                            continue;
                            break;
                        case param_idx::unused:
                        case param_idx::sspi:
                            if (pt == pass_type::offset_size_table) {
                                tds_ctx.write(0_tdsu32);
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
                            TDSLITE_ASSERT(false);
                            TDSLITE_UNREACHABLE;
                        } break;
                    } // ... switch (static_cast<e_tds_login_parameter_idx>(j))

                    TDSLITE_ASSERT(tw);

                    if (pt == pass_type::offset_size_table) {
                        // We're filling the offset table (first pass)
                        tds_ctx.write_le(current_string_offset);
                        // Character count, not length in bytes
                        tds_ctx.write_le(static_cast<tdsl::uint16_t>((*tw).size()));

                        current_string_offset += string_parameter_writer::calculate_write_size((*tw));
                    }
                    else {

                        // Client ID is offset section only.
                        if (pi == param_idx::client_id) {
                            continue;
                        }

                        // Write the string itself.
                        if (*tw) {
                            string_parameter_writer::write(tds_ctx, *tw, static_cast<e_tds_login_parameter_idx>(pi));
                            string_section_size += string_parameter_writer::calculate_write_size((*tw));
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
            // Write the login packet length first
            put_login_header_length(total_packet_data_size);
            // ... then, the TDS packet length
            tds_ctx.put_tds_header_length(total_packet_data_size);
            // Send the packet.
            tds_ctx.send();
            // on_data_recv
        } // ... void do_login_impl(const LoginParamsType & params) noexcept {

        static void handle_response(void * self, tdsl::span<const tdsl::uint8_t> resp) {
            (void) self;
            (void) resp;
            // should expect what?
            // Invoke login callback
        }

    private:
        tds_context_type & tds_ctx;
        login_callback_type login_callback;
    };
}} // namespace tdsl::detail