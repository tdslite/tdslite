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

#include <tdslite/net/base/msg_callback.hpp>
#include <tdslite/detail/tdsl_message_type.hpp>
#include <tdslite/detail/tdsl_message_token_type.hpp>
#include <tdslite/detail/tdsl_envchange_type.hpp>

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_binary_reader.hpp>

namespace tdsl { namespace net {

    struct network_impl_base {

        network_impl_base() noexcept {
            // Disabled by default
            flags.expect_full_tds_pdu = {false};
        }

        /**
         * TDS response message handler function
         *
         * @param [in] data The received data
         *
         * @returns The amount of bytes needed for next response call
         */
        tdsl::uint32_t handle_tds_response(tdsl::span<const tdsl::uint8_t> data) noexcept {
            // Response reader
            tdsl::binary_reader<tdsl::endian::big> rr{data.data(), data.size()};

            constexpr static auto k_tds_hdr_len = 8;

            if (not rr.has_bytes(/*amount_of_bytes=*/k_tds_hdr_len)) {
                return k_tds_hdr_len - rr.size_bytes(); // need at least 8 bytes
            }

            using msg_type    = detail::e_tds_message_type;

            const auto mt     = rr.read<tdsl::uint8_t>();
            const auto status = rr.read<tdsl::uint8_t>();
            const auto length = rr.read<tdsl::uint16_t>();

            // Skip channel, packet id and window
            rr.advance(/*amount_of_bytes=*/4);

            (void) status;

            if (flags.expect_full_tds_pdu && not(rr.size_bytes() >= length)) {
                // Ensure that we got the whole intended payload
                return length - rr.size_bytes();
            }

            if (msg_cb_ctx.callback) {
                auto need_bytes = msg_cb_ctx.callback(msg_cb_ctx.user_ptr, static_cast<msg_type>(mt), rr.read(rr.remaining_bytes()));
                return (need_bytes == 0 ? k_tds_hdr_len : need_bytes);
            }

            return {};
        }

        /**
         * Set receive callback
         *
         * @param [in] rcb New callback
         */
        TDSLITE_SYMBOL_VISIBLE void register_msg_recv_callback(void * user_ptr, msg_callback_fn_type rcb) {
            msg_cb_ctx.user_ptr = user_ptr;
            msg_cb_ctx.callback = rcb;
        }

    protected:
        msg_callback_ctx msg_cb_ctx{};
        struct {
            bool expect_full_tds_pdu : 1;
            bool reserved : 7;
        } flags;

    private:
        // void
    };
}} // namespace tdsl::net