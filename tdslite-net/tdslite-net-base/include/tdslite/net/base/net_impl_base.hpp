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

#include <tdslite/detail/tdsl_callback.hpp>
#include <tdslite/detail/tdsl_message_type.hpp>
#include <tdslite/detail/tdsl_message_token_type.hpp>
#include <tdslite/detail/tdsl_envchange_type.hpp>
#include <tdslite/detail/tdsl_packet_handler_result.hpp>

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_debug_print.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>

namespace tdsl { namespace net {

    namespace detail {

        template <typename T>
        using has_recv_member_fn_t = decltype(traits::declval<T>().do_recv(static_cast<tdsl::uint32_t>(0)));

        template <typename T>
        using has_recv_member_fn = traits::is_detected<has_recv_member_fn_t, T>;

        // template <typename T>
        // using has_recv_buf_member_fn_t = decltype(traits::declval<T>().recv_buf());

        // template <typename T>
        // using has_recv_buf_member_fn = traits::is_detected<has_recv_buf_member_fn_t, T>;
    } // namespace detail

    template <typename ConcreteNetImpl>
    struct network_impl_base {
    private:
        inline void call_recv(tdsl::uint32_t min_amount) {
            static_cast<ConcreteNetImpl &>(*this).do_recv(min_amount);
        }

        // consume_recv_buffer(tdsl::uint32_t, tdsl::uint32_t)
        // span<>

    public:
        using tds_msg_handler_result = packet_handler_result<bool, false>;

        // Message callback context
        using msg_callback_ctx       = callback<void, tdsl::uint32_t (*)(/*user_ptr*/ void *, /*msg_type*/ tdsl::detail::e_tds_message_type,
                                                                   /*msg*/ tdsl::span<const tdsl::uint8_t>)>;

        enum class e_conection_state : tdsl::int8_t
        {
            attempt_in_progress = 0,
            connected           = 1,
            disconnected        = -1,
            connection_failed   = -2,
            resolve_failed      = -3,
        };

        using connection_state_callback_ctx = callback<void, void (*)(/*user_ptr*/ void *, e_conection_state)>;

        network_impl_base() noexcept {
            // If you are hitting this static assertion, it means either your ConcreteNetImpl
            // does not have do_recv function, or it does not have the expected function signature.
            static_assert(traits::dependent_bool<detail::has_recv_member_fn<ConcreteNetImpl>::value>::value,
                          "The type ConcreteNetImpl must implement void do_recv(tdsl::uint32_t) function!");

            // static_assert(traits::dependent_bool<detail::has_recv_buf_member_fn<ConcreteNetImpl>::value>::value,
            //               "The type ConcreteNetImpl must implement tdsl::span<const tdsl::uint8_t> recv_buf() function!");
            // Disabled by default
            flags.expect_full_tds_pdu = {false};
        }

        // begin_recv_tds_response

        /**
         * Receive one, complete TDS PDU.
         */
        void receive_tds_pdu() {

            // Each TDS message contains a status field
            // TDS payload can fragment between multiple messages
            // Each fragment will contain a TDS header
            // Last fragment will have its EOM bit set, others will not

            // while(not eom_seen)<--------------------------|
            //  read(8)                                      |
            //  read_tds_header                              |
            //  read_current_fragment(as a whole)            |
            //  remove read bytes from buf                   |
            //  pass lower stack                             |
            //  remove consumed bytes from beginning         |
            //  shift remaining bytes to start of the buffer |
            //  repeat ---------------------------------------

            // ... or alternatively, find a way to make a circular
            // buffer contiguous.

            // while EOM not received,
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

            // tds_msg_handler_result result{};

            constexpr static auto k_tds_hdr_len = 8;

            if (not rr.has_bytes(/*amount_of_bytes=*/k_tds_hdr_len)) {
                TDSL_DEBUG_PRINT("network_impl_base::handle_tds_response(...) -> insufficient data, need %d more byte(s): \n",
                                 (k_tds_hdr_len - rr.size_bytes()));
                return k_tds_hdr_len - rr.size_bytes();
                //  return result; // need at least 8 bytes
            }

            using msg_type    = tdsl::detail::e_tds_message_type;

            const auto mt     = rr.read<tdsl::uint8_t>();
            const auto status = rr.read<tdsl::uint8_t>();
            const auto length = rr.read<tdsl::uint16_t>();

            // Skip channel, packet id and window
            rr.advance(/*amount_of_bytes=*/4);

            (void) status; // FIXME: Check EOM

            if (flags.expect_full_tds_pdu && not(rr.size_bytes() >= length)) {
                // Ensure that we got the whole intended payload
                TDSL_DEBUG_PRINT("network_impl_base::handle_tds_response(...) -> insufficient data, need %d more byte(s): \n",
                                 (length - rr.size_bytes()));
                return length - rr.size_bytes();
            }

            // poll more data here?

            // done token?

            if (msg_cb_ctx.callback) {
                auto need_bytes = msg_cb_ctx.callback(msg_cb_ctx.user_ptr, static_cast<msg_type>(mt), rr.read(rr.remaining_bytes()));
                TDSL_DEBUG_PRINT("network_impl_base::handle_tds_response(...) -> msg_cb_ctx, need byte(s) value:%d \n",
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
        TDSL_SYMBOL_VISIBLE void register_msg_recv_callback(void * user_ptr, msg_callback_ctx::function_type cb) {
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