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

#include <tdslite/detail/tds_inttypes.hpp>
#include <tdslite/detail/tds_macrodef.hpp>
#include <tdslite/detail/tds_span.hpp>
#include <tdslite/detail/tds_type_traits.hpp>
#include <stdio.h>

namespace tdslite { namespace tds {

    template <typename Derived>
    struct net_packet_xmit_context {
        //
        template <typename T, traits::enable_if_integral<T> = true>
        inline auto write(T v) noexcept -> void {
            tdslite::span<const tdslite::uint8_t> data(reinterpret_cast<const tdslite::uint8_t *>(&v), sizeof(T));
            write(data);
            // this->write(data);
        }

        inline void write(tdslite::span<const tdslite::uint8_t> data) noexcept {
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
         * Wide-character (16 bit) string view
         */
        using wstring_view = tdslite::span<const char16_t>;

        /**
         * String view
         */
        using string_view  = tdslite::span<const char>;

        /**
         * Attempt to login into database @p dbname database at @p host with account credentials
         * @p username and @p password.
         *
         * @note Parameters on this overload are wide-character strings.
         *
         * @param [in] host Database engine hostname (or IP address)
         * @param [in] username Name of the user to login
         * @param [in] password Password of user
         * @param [in] dbname Database name
         */
        void do_login(wstring_view host, wstring_view username, wstring_view password, wstring_view dbname) {
            printf("called\n");
            this->template write(0x10_tdsu8);
            (void) host;
            (void) username;
            (void) password;
            (void) dbname;
        }

        /**
         * Attempt to login into database @p dbname database at @p host with account credentials
         * @p username and @p password.
         *
         * @note Parameters on this overload are single-byte character strings.
         *
         * @param [in] host Database engine hostname (or IP address)
         * @param [in] username Name of the user to login
         * @param [in] password Password of user
         * @param [in] dbname Database name
         */
        void do_login(string_view host, string_view username, string_view password, string_view dbname) {
            // Login opcode: 0x10
            this->template write(0x10_tdsu8);
            (void) host;
            (void) username;
            (void) password;
            (void) dbname;
        }

    private:
        friend class net_packet_xmit_context<login_context<NetImpl>>;
        friend class net_packet_recv_context<login_context<NetImpl>>;

        // response callback?
    };
}} // namespace tdslite::tds