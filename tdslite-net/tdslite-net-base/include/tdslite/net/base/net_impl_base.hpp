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

#include <tdslite/detail/tdsl_callback_context.hpp>
#include <tdslite/detail/tdsl_message_type.hpp>
#include <tdslite/detail/tdsl_message_token_type.hpp>
#include <tdslite/detail/tdsl_envchange_type.hpp>

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_debug_print.hpp>

namespace tdsl { namespace net {

    struct network_impl_base {

        // Message callback context
        using msg_callback_ctx = callback_context<void, tdsl::uint32_t (*)(/*user_ptr*/ void *, /*msg_type*/ detail::e_tds_message_type,
                                                                           /*msg*/ tdsl::span<const tdsl::uint8_t>)>;

        enum class e_conection_state : tdsl::int8_t
        {
            attempt_in_progress = 0,
            connected           = 1,
            disconnected        = -1,
            connection_failed   = -2,
            resolve_failed      = -3,
        };

        using connection_state_callback_ctx = callback_context<void, void (*)(/*user_ptr*/ void *, e_conection_state)>;

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
                TDSLITE_DEBUG_PRINT("network_impl_base::handle_tds_response(...) -> insufficient data, need %d more byte(s): \n",
                                    (k_tds_hdr_len - rr.size_bytes()));
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
                TDSLITE_DEBUG_PRINT("network_impl_base::handle_tds_response(...) -> insufficient data, need %d more byte(s): \n",
                                    (length - rr.size_bytes()));
                return length - rr.size_bytes();
            }

            if (msg_cb_ctx.callback) {
                auto need_bytes = msg_cb_ctx.callback(msg_cb_ctx.user_ptr, static_cast<msg_type>(mt), rr.read(rr.remaining_bytes()));
                TDSLITE_DEBUG_PRINT("network_impl_base::handle_tds_response(...) -> msg_cb_ctx, need byte(s) value:%d \n",
                                    (length - rr.size_bytes()));
                return need_bytes;
            }

            return {};
        }

        /**
         * Set receive callback
         *
         * @param [in] cb New callback
         */
        TDSLITE_SYMBOL_VISIBLE void register_msg_recv_callback(void * user_ptr, msg_callback_ctx::function_type cb) {
            msg_cb_ctx.user_ptr = user_ptr;
            msg_cb_ctx.callback = cb;
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