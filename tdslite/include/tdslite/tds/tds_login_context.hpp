/**
 * _________________________________________________
 * Tabular Data Stream protocol login operations
 *
 * @file   login_context.hpp
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
#include <tdslite/tds/tds_opcode.hpp>
#include <tdslite/tds/tds_flag_types.hpp>

namespace tdslite { namespace tds {

    template <typename Derived>
    struct net_packet_xmit_context {
        //
        template <typename T, traits::enable_if_integral<T> = true>
        inline auto write(T v) noexcept -> void {
            tdslite::span<const tdslite::uint8_t> data(reinterpret_cast<const tdslite::uint8_t *>(&v), sizeof(T));
            write(data);
        }

        template <typename T>
        inline void write(tdslite::span<T> data) noexcept {
            static_cast<Derived &>(*this).do_write(data);
        }

        template <typename... Args>
        inline void send(Args &&... args) noexcept {
            static_cast<Derived &>(*this)->do_send(TDSLITE_FORWARD(args)...);
        }
    };

    template <typename Derived>
    struct net_packet_recv_context {
        template <typename... Args>
        inline void recv(Args &&... args) {
            static_cast<Derived &>(*this)->do_recv(TDSLITE_FORWARD(args)...);
        }
    };

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
    struct login_context : NetImpl,
                           private net_packet_xmit_context<login_context<NetImpl>>,
                           private net_packet_recv_context<login_context<NetImpl>> {

        /**
         * Wide-character (16 bit) string view
         */
        using wstring_view = tdslite::span<const char16_t>;

        /**
         * String view
         */
        using string_view  = tdslite::span<const char>;

        /**
         * The arguments for the login operation
         *
         * @tparam StringViewType View type for string variables (default: string_view)
         */
        template <typename StringViewType = string_view>
        struct login_parameters_type {
            StringViewType server_name;          // Target server hostname or IP address
            StringViewType db_name;              // Target database name
            StringViewType user_name;            // Database user name
            StringViewType password;             // Database user password
            StringViewType app_name;             // Client application name
            StringViewType client_name{""};      // The client machine name
            tdslite::uint32_t packet_size{4096}; // Desired packet size (default = 4096)
        };

        /**
         * Login parameters (single-character)
         */
        using login_parameters  = login_parameters_type<string_view>;

        /**
         * Login parameters (wide-character)
         */
        using wlogin_parameters = login_parameters_type<wstring_view>;

        /*
            Example Login7 packet:

            10 01 00 90 00 00 01 00 88 00 00 00 02 00 09 72
            00 10 00 00 00 00 00 07 00 01 00 00 00 00 00 00
            E0 03 00 00 00 00 00 00 09 04 00 00 5E 00 08 00
            6E 00 02 00 72 00 00 00 72 00 07 00 80 00 00 00
            80 00 00 00 80 00 04 00 88 00 00 00 88 00 00 00
            00 50 8B E2 B7 8F 88 00 00 00 88 00 00 00 88 00
            00 00 00 00 00 00 73 00 6B 00 6F 00 73 00 74 00
            6F 00 76 00 31 00 73 00 61 00 4F 00 53 00 51 00
            4C 00 2D 00 33 00 32 00 4F 00 44 00 42 00 43 00
        */

        /**
         * Attempt to login into the database engine with the specified login parameters
         *
         * @note Parameters on this overload are single-byte character strings.
         *
         * @param [in] params Login parameters
         */
        void do_login(const wlogin_parameters & params) {
            this->template write(0x10_tdsu8);
            (void) params;
        }

        // SQLardUtil::sqlard_write_le<uint32_t>(buf, offset, m_uiLength);
        // SQLardUtil::sqlard_write_le<uint32_t>(buf, offset, m_uiTDSVersion);
        // SQLardUtil::sqlard_write_le<uint32_t>(buf, offset, m_uiPacketSize);
        // SQLardUtil::sqlard_write_le<uint32_t>(buf, offset, m_uiClientProgVer);
        // SQLardUtil::sqlard_write_le<uint32_t>(buf, offset, m_uiClientPID);
        // SQLardUtil::sqlard_write_le<uint32_t>(buf, offset, m_uiConnectionID);
        // SQLardUtil::sqlard_write_le<uint8_t>(buf, offset, m_ubOptionFlags1);
        // SQLardUtil::sqlard_write_le<uint8_t>(buf, offset, m_ubOptionFlags2);
        // SQLardUtil::sqlard_write_le<uint8_t>(buf, offset, m_ubTypeFlags);
        // SQLardUtil::sqlard_write_le<uint8_t>(buf, offset, m_ubOptionFlags3);
        // SQLardUtil::sqlard_write_le<uint32_t>(buf, offset, m_uiClientTimeZone);
        // SQLardUtil::sqlard_write_le<uint32_t>(buf, offset, m_uiClientLCID);

        // m_uiLength = 0;
        // m_uiTDSVersion = 0x70000000; /* TDS 7.0 */
        // m_uiPacketSize = 4096;
        // m_uiClientProgVer = 117440512;
        // m_uiConnectionID = 0;
        // m_uiClientPID = 256;
        // m_uiClientTimeZone = 0x000001e0;
        // m_uiClientLCID = 0x00000409;
        // m_ubOptionFlags1 = 0xE0;
        // m_ubOptionFlags2 = 0x03;
        // m_ubOptionFlags3 = 0x00;
        // m_ubTypeFlags = 0x00;

        /**
         * Attempt to login into the database engine with the specified login parameters
         *
         * @note Parameters on this overload are single-byte character strings.
         *
         * @param [in] params Login parameters
         */
        void do_login(const login_parameters & params) {
            this->template write(static_cast<tdslite::uint8_t>(e_tds_opcode::login)); // OPCODE
            this->template write<tdslite::uint32_t>(0_tdsu32);                        // placeholder for packet length
            this->template write(
                detail::host_to_network(static_cast<tdslite::uint32_t>(e_tds_version::sql_server_2000_sp1))); // TDS version
            this->template write(params.packet_size); // Requested packet size by the client
            this->template write(100101_tdsu32);      // Client program version
            this->template write(0_tdsu32);           // Client program PID
            this->template write(0_tdsu32);           // Connection ID

            // // Default option flags
            // constexpr option_flags_1 opt1 = []() {
            //     option_flags_1 of{};
            //     of.set_lang = 1;
            //     of.database = 1;
            //     of.use_db   = 1;
            //     return of;
            // }();

            // constexpr option_flags_2 opt2 = []() {
            //     option_flags_2 of{};
            //     of.set_lang = 1;
            //     of.database = 1;
            //     of.use_db   = 1;
            //     return of;
            // }();

            // constexpr option_flags_3 opt3 = []() {
            //     option_flags_3 of{};
            //     of.set_lang = 1;
            //     of.database = 1;
            //     of.use_db   = 1;
            //     return of;
            // }();

            // constexpr type_flags tf = []() {
            //     type_flags tf{};
            //     tf.set_lang = 1;
            //     tf.database = 1;
            //     tf.use_db   = 1;
            //     return tf;
            // }();

            this->template write(0xE0_tdsu8); // Option Flags (1)
            this->template write(0x03_tdsu8); // Option Flags (2)
            this->template write(0x00_tdsu8); // Type Flags
            this->template write(0x00_tdsu8); // Option Flags (3)
            this->template write(0_tdsu32);   // Client Timezone (unused)
            this->template write(0_tdsu32);   // Client language code ID

            static constexpr tdslite::uint8_t parameter_count          = 9; // for tds version sql_server_2000_sp1
            static constexpr tdslite::uint32_t tds_pkthdr_size         = 8;
            static constexpr tdslite::uint32_t login_pkthdr_size       = 36;
            static constexpr tdslite::uint32_t client_id_size          = 6;
            static constexpr tdslite::uint32_t len_offset_section_size = (sizeof(tdslite::uint16_t) * 2) * parameter_count;

            constexpr tdslite::uint32_t offset_table_start = login_pkthdr_size + tds_pkthdr_size + len_offset_section_size + client_id_size;

            enum class e_tds_login_parameter_idx : tdslite::uint8_t
            {
                client_name   = 0,
                user_name     = 1,
                password      = 2,
                app_name      = 3,
                server_name   = 4,
                unused        = 5,
                library_name  = 6,
                locale        = 7,
                database_name = 8,
            };

            tdslite::uint16_t current_offset = offset_table_start;

            // Two-pass operation:
            // Pass 0: Fill the offset/length table
            // Pass 1: Fill the strings
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < parameter_count; j++) {
                    // Write offset and length values for each extension
                    const string_view * tw = {nullptr};
                    switch (static_cast<e_tds_login_parameter_idx>(j)) {
                        case e_tds_login_parameter_idx::client_name:
                            tw = &params.client_name;
                            break;
                        case e_tds_login_parameter_idx::user_name:
                            tw = &params.user_name;
                            break;
                        case e_tds_login_parameter_idx::password:
                            tw = &params.password;
                            break;
                        case e_tds_login_parameter_idx::app_name:
                            tw = &params.app_name;
                            break;
                        case e_tds_login_parameter_idx::server_name:
                            tw = &params.server_name;
                            break;
                        case e_tds_login_parameter_idx::database_name:
                            tw = &params.db_name;
                            break;
                        case e_tds_login_parameter_idx::library_name:
                            // FIXME: This does not belong here
                            static constexpr char library_name []        = {'t', 'd', 's', 'l', 'i', 't', 'e'};
                            static constexpr string_view library_name_sv = library_name;
                            tw                                           = &library_name_sv;
                            break;
                        case e_tds_login_parameter_idx::unused:
                        case e_tds_login_parameter_idx::locale:
                            // offset(u16), size(u16)
                            this->template write(0_tdsu32);
                            continue;
                        default: {
                            TDSLITE_ASSERT(false);
                            TDSLITE_UNREACHABLE;
                        } break;
                    }
                    TDSLITE_ASSERT(tw);

                    if (i == 0) {
                        // We're filling the offset table (first pass)
                        this->template write<tdslite::uint16_t>(current_offset);
                        this->template write<tdslite::uint16_t>(tw->size_bytes());
                        current_offset += tw->size_bytes();
                    }
                    else {
                        if (*tw) {
                            // We're filling the strings
                            this->template write((*tw));
                        }
                    }
                }
                if (i == 0) {
                    // Between the offset table and strings, there is
                    // 6 octet client id.
                    this->template write<tdslite::uint32_t>(0);
                    this->template write<tdslite::uint16_t>(0);
                }
            }
        }

    private:
        friend struct net_packet_xmit_context<login_context<NetImpl>>;
        friend struct net_packet_recv_context<login_context<NetImpl>>;

        // response callback?
    };
}} // namespace tdslite::tds