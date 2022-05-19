/**
 * _________________________________________________
 *
 * @file   generic_data_callback_defn.hpp
 * @author Mustafa Kemal GILOR <mgilor@nettsi.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/detail/tdsl_callback_ctx.hpp>
#include <tdslite/detail/tdsl_message_type.hpp>

namespace tdsl { namespace net {

    /**
     * The function type of the callback function that is going to be invoked
     * when data is received in a network implementation
     */
    using msg_callback_fn_type = tdsl::uint32_t (*)(/*user_ptr*/ void *, /*msg_type*/ detail::e_tds_message_type,
                                                    /*msg*/ tdsl::span<const tdsl::uint8_t>);

    // Data callback context
    using msg_callback_ctx     = callback_ctx<msg_callback_fn_type>;

}} // namespace tdsl::net