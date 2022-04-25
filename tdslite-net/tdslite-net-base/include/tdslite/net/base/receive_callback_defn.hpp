/**
 * _________________________________________________
 *
 * @file   receive_callback_defn.hpp
 * @author Mustafa Kemal GILOR <mgilor@nettsi.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>

namespace tdsl { namespace net {

    /**
     * The function type of the callback function that is going to be invoked
     * when data is received in a network implementation
     */
    using receive_callback_fn_type = void (*)(void *, tdsl::span<const tdsl::uint8_t>);

    /**
     * Receive callback context
     */
    struct receive_callback_ctx {
        void * user_ptr{nullptr};                   // Will be passed to the callback function as first argument
        receive_callback_fn_type callback{nullptr}; // The callback function
    };

}} // namespace tdsl::net