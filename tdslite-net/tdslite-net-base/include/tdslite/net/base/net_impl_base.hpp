/**
 * _________________________________________________
 *
 * @file   net_impl_base.hpp
 * @author Mustafa Kemal GILOR <mgilor@nettsi.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/net/base/receive_callback_defn.hpp>
#include <tdslite/detail/tdsl_message_type.hpp>

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_binary_reader.hpp>

namespace tdsl { namespace net {

    struct network_impl_base {

        /**
         * Set receive callback
         *
         * @param [in] rcb New callback
         */
        TDSLITE_SYMBOL_VISIBLE void do_set_receive_callback(void * user_ptr, receive_callback_fn_type rcb) {
            recv_cb_ctx.user_ptr = user_ptr;
            recv_cb_ctx.callback = rcb;
        }

        /**
         * TDS response message handler function
         *
         * @param [in] data The received data
         *
         * @returns The amount of bytes needed for next response call
         */
        tdsl::uint32_t handle_tds_response(tdsl::span<tdsl::uint8_t> data) noexcept {
            // Response reader
            tdsl::binary_reader<tdsl::endian::big> rr{data.data(), data.size()};
            if (not rr.has_bytes(8 /*tds header*/)) {
                return 8; //
            }

            auto type   = rr.read<tdsl::uint8_t>();
            auto status = rr.read<tdsl::uint8_t>();
            auto length = rr.read<tdsl::uint32_t>();

            (void) status;

            if (not(rr.size_bytes() >= length)) {
                return length - rr.size_bytes();
            }

            switch (static_cast<detail::e_tds_message_type>(type)) {
                case detail::e_tds_message_type::tabular_result: {
                    while (true) {
                    }
                } break;
            }
        }

        // void handle_tabular_result()

    protected:
        receive_callback_ctx recv_cb_ctx{};
    };

}} // namespace tdsl::net