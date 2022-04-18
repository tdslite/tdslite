/**
 * _________________________________________________
 * Tabular Data Stream protocol login operations
 *
 * @file   tds_login_context.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   14.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/detail/tds_inttypes.hpp>
#include <tdslite/detail/tds_macrodef.hpp>
#include <tdslite/detail/tds_span.hpp>
#include <tdslite/detail/tds_type_traits.hpp>
#include <tdslite/detail/tds_byte_swap.hpp>
#include <tdslite/tds/ms_lang_code_id.hpp>
#include <tdslite/tds/tds_version.hpp>
#include <tdslite/tds/tds_message_type.hpp>
#include <tdslite/tds/tds_flag_types.hpp>

#include <stdio.h>

namespace tdslite { namespace tds {

    /**
     * Network packet transmit context
     *
     * @tparam Derived Derived type (CRTP)
     */
    template <typename Derived>
    struct net_packet_xmit_context {
        //
        template <typename T, traits::enable_if_integral<T> = true>
        inline auto write(T v) noexcept -> void {
            tdslite::span<const tdslite::uint8_t> data(reinterpret_cast<const tdslite::uint8_t *>(&v), sizeof(T));
            write(data);
        }

        template <typename T, traits::enable_if_integral<T> = true>
        inline auto write(tdslite::uint32_t offset, T v) noexcept -> void {
            tdslite::span<const tdslite::uint8_t> data(reinterpret_cast<const tdslite::uint8_t *>(&v), sizeof(T));
            write(offset, data);
        }

        template <typename T>
        inline void write(tdslite::uint32_t offset, tdslite::span<T> data) noexcept {
            static_cast<Derived &>(*this).do_write(offset, data);
        }

        template <typename T>
        inline void write(tdslite::span<T> data) noexcept {
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
            static_cast<Derived &>(*this)->do_recv(TDSLITE_FORWARD(args)...);
        }
    };

    // --------------------------------------------------------------------------------

    /**
     * Base type for all TDS message contexts
     *
     * @tparam NetImpl Network-layer Implementation
     * @tparam TYPE Message type
     */
    template <typename NetImpl, e_tds_message_type TYPE>
    struct tds_context : public NetImpl,
                         protected net_packet_xmit_context<tds_context<NetImpl, TYPE>>,
                         protected net_packet_recv_context<tds_context<NetImpl, TYPE>> {
        using xmit_context = net_packet_xmit_context<tds_context<NetImpl, TYPE>>;
        using recv_context = net_packet_recv_context<tds_context<NetImpl, TYPE>>;

        struct tds_header {
            tdslite::uint8_t type;
            tdslite::uint8_t status;
            tdslite::uint16_t length;
            tdslite::uint16_t channel;
            tdslite::uint8_t packet_number;
            tdslite::uint8_t window;
        } TDSLITE_PACKED;

        struct tds_header_offsets {
            static constexpr tdslite::uint16_t type          = 0;
            static constexpr tdslite::uint16_t status        = type + sizeof(tdslite::uint8_t);
            static constexpr tdslite::uint16_t length        = status + sizeof(tdslite::uint8_t);
            static constexpr tdslite::uint16_t channel       = length + sizeof(tdslite::uint16_t);
            static constexpr tdslite::uint16_t packet_number = channel + sizeof(tdslite::uint16_t);
            static constexpr tdslite::uint16_t window        = packet_number + sizeof(tdslite::uint8_t);
        };

        /**
         *
         *
         */
        inline void write_tds_header() noexcept {
            this->template write(static_cast<tdslite::uint8_t>(TYPE)); // Packet type
            this->template write(0x01_tdsu8);                          // STATUS
            this->template write(0_tdsu16);                            // Placeholder for length
            this->template write(0_tdsu32);                            // Channel, Packet ID and Window
        }

        /**
         *
         *
         * @param data_length
         */
        void put_tds_header_length(tdslite::uint16_t data_length) noexcept {
            // Length is the size of the packet inclusing the 8 bytes in the packet header.
            // It is the number of bytes from start of this header to the start of the next packet header.
            // Length is a 2-byte, unsigned short and is represented in network byte order (big-endian).
            this->template write(TDSLITE_OFFSETOF(tds_header, length),
                                 detail::host_to_network(static_cast<tdslite::uint16_t>(data_length + 8)));
        }

    private:
        friend struct net_packet_xmit_context<tds_context<NetImpl, TYPE>>;
        friend struct net_packet_recv_context<tds_context<NetImpl, TYPE>>;
    };

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
    struct login_context : public tds_context<NetImpl, e_tds_message_type::login> {

        using typename tds_context<NetImpl, e_tds_message_type::login>::xmit_context;

        /**
         * Wide-character (16 bit) string view
         */
        struct wstring_view : public tdslite::span<const char16_t> {

            using ::tdslite::span<const char16_t>::span;

            wstring_view() : span() {}

            template <tdslite::uint32_t N>
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
        struct string_view : public tdslite::span<const char> {
            using ::tdslite::span<const char>::span;

            string_view() : span() {}

            template <tdslite::uint32_t N>
            string_view(const char (&str) [N]) : span(str, N) {
                // If the string is NUL-terminated, omit the NUL terminator.
                if (N > 0 && str [N - 1] == '\0') {
                    size_ -= 1;
                }
            }
        };

        /**
         * The arguments for the login operation
         *
         * @tparam StringViewType View type for string variables (default: string_view)
         */
        template <typename StringViewType = string_view>
        struct login_parameters_type {
            using sv_type = StringViewType;
            StringViewType server_name;             // Target server hostname or IP address
            StringViewType db_name;                 // Target database name
            StringViewType user_name;               // Database user name
            StringViewType password;                // Database user password
            StringViewType app_name;                // Client application name
            StringViewType client_name{""};         // The client machine name
            StringViewType library_name{"tdslite"}; // The library name
            tdslite::uint32_t packet_size{4096};    // Desired packet size (default = 4096)
        };

        /**
         * Login parameters (single-character)
         */
        using login_parameters  = login_parameters_type<string_view>;

        /**
         * Login parameters (wide-character)
         */
        using wlogin_parameters = login_parameters_type<wstring_view>;

        enum class e_tds_login_parameter_idx : tdslite::uint8_t
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

        constexpr static tdslite::uint16_t calc_sizeof_offset_size_section() noexcept {
            return ((static_cast<tdslite::uint16_t>(e_tds_login_parameter_idx::end) - 1) * sizeof(tdslite::uint32_t)) +
                   6 /*client id size*/;
        }

        /**
         * Encode a password for TDS protocol
         *
         * @param [in] buf Buffer to encode
         */
        template <tdslite::uint32_t N>
        static inline void encode_password(tdslite::uint8_t (&buf) [N]) noexcept {
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
            static void write(xmit_context & xc, const string_view & sv, e_tds_login_parameter_idx parameter_type) {
                // We're filling the strings
                for (auto ch : sv) {
                    char16_t c = ch;
                    e_tds_login_parameter_idx::password == parameter_type
                        ? encode_password(reinterpret_cast<tdslite::uint8_t(&) [sizeof(char16_t)]>(ch))
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
            static void write(xmit_context & xc, const wstring_view & sv, e_tds_login_parameter_idx parameter_type) {

                if (not(e_tds_login_parameter_idx::password == parameter_type)) {
                    login_context::write(sv);
                    return;
                }

                // Password needs special treatment before sending.
                for (auto ch : sv) {
                    char16_t c = ch;
                    encode_password(reinterpret_cast<tdslite::uint8_t(&) [sizeof(char16_t)]>(ch));
                    xc.write(c);
                }
            }

            /**
             * Calculate the size of the written output for given @p s
             *
             * @param [in] s String to calculate the write size of
             * @return tdslite::uint32_t The total bytes would've written fo @p s on a write call
             */
            static constexpr auto write_size(const string_view & s) noexcept -> tdslite::uint32_t {
                return s.size_bytes() * 2;
            }

            /**
             * Calculate the size of the written output for given @p s
             *
             * @param [in] s String to calculate the write size of
             * @return tdslite::uint32_t The total bytes would've written fo @p s on a write call
             */
            static constexpr auto write_size(const wstring_view & s) noexcept -> tdslite::uint32_t {
                return s.size_bytes();
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
        inline void do_login(const wlogin_parameters & params) noexcept {
            do_login_impl<wlogin_parameters>(params);
        }

        // --------------------------------------------------------------------------------

        /**
         * Attempt to login into the database engine with the specified login parameters
         *
         * @note Parameters on this overload are single-byte character strings.
         *
         * @param [in] params Login parameters
         */
        inline void do_login(const login_parameters & params) noexcept {
            do_login_impl<login_parameters>(params);
        }

    private:
        /**
         * Attempt to login into the database engine with the specified login parameters
         *
         * @note Parameters on this overload are single-byte character strings.
         *
         * @param [in] params Login parameters
         */
        template <typename LoginParamsType>
        void do_login_impl(const LoginParamsType & params) noexcept {
            this->write_tds_header();
            this->template write(0_tdsu32); // placeholder for packet length
            this->template write(
                detail::host_to_network(static_cast<tdslite::uint32_t>(e_tds_version::sql_server_2000_sp1))); // TDS version
            this->template write(params.packet_size); // Requested packet size by the client
            this->template write(100101_tdsu32);      // Client program version
            this->template write(0_tdsu32);           // Client program PID
            this->template write(0_tdsu32);           // Connection ID
            this->template write(0xE0_tdsu8);         // Option Flags (1)
            this->template write(0x03_tdsu8);         // Option Flags (2)
            this->template write(0x00_tdsu8);         // Type Flags
            this->template write(0x00_tdsu8);         // Option Flags (3)
            this->template write(0_tdsu32);           // Client Timezone (unused)
            this->template write(0_tdsu32);           // Client language code ID

            static constexpr tdslite::uint32_t tds_pkthdr_size    = 8;
            static constexpr tdslite::uint32_t login_pkthdr_size  = 36;

            // Calculate the total packet data section size.
            tdslite::uint16_t total_packet_data_size              = login_pkthdr_size + calc_sizeof_offset_size_section();
            // Calculate the starting positiof of the offset/size table. This will act as base
            // offset for the offset/size table's offset values.
            constexpr tdslite::uint32_t string_table_offset_start = login_pkthdr_size + tds_pkthdr_size + calc_sizeof_offset_size_section();

            // Indicates the offset of the current string in the string table
            tdslite::uint16_t current_string_offset               = string_table_offset_start;

            // Length of the string section in the data packet (sum of all string lengths)
            tdslite::uint16_t string_section_size                 = 0;

            // We will fill the offset/length table and string table in two passes:
            // Pass 0: Fill the offset/length table
            // Pass 1: Fill the strings
            enum class pass_type : tdslite::uint8_t
            {
                begin             = 0,
                offset_size_table = begin,
                string_table,
                end
            };
            using param_idx = e_tds_login_parameter_idx;

            printf("offset size section : %d\n", calc_sizeof_offset_size_section());

            for (pass_type pt = pass_type::begin; pt < pass_type::end; pt = static_cast<pass_type>(static_cast<tdslite::uint8_t>(pt) + 1)) {
                for (param_idx pi = param_idx::begin; pi < param_idx::end;
                     pi           = static_cast<param_idx>(static_cast<tdslite::uint8_t>(pi) + 1)) {
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
                                this->template write(0_tdsu16);
                            }
                        // fallthrough
                        case param_idx::unused:
                        case param_idx::locale:
                        case param_idx::sspi:
                        case param_idx::atchdbfile:

                            if (pt == pass_type::offset_size_table) {
                                // offset(u16), size(u16)
                                this->template write(0_tdsu32);
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
                        this->template write<tdslite::uint16_t>(current_string_offset);
                        this->template write<tdslite::uint16_t>(string_parameter_writer::write_size(*tw));
                        current_string_offset += string_parameter_writer::write_size(*tw);
                    }
                    else {

                        // Client ID is offset section only.
                        if (pi == param_idx::client_id) {
                            continue;
                        }

                        // Write the string itself.
                        if (*tw) {
                            string_parameter_writer::write(*this, *tw, static_cast<e_tds_login_parameter_idx>(pi));
                            string_section_size += string_parameter_writer::write_size(*tw);
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
            this->template put_tds_header_length(total_packet_data_size);

            // Send the packet.
            this->send();
        } // ... void do_login_impl(const LoginParamsType & params) noexcept {

        // response callback?
    };
}} // namespace tdslite::tds