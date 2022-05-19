/**
 * _________________________________________________
 *
 * @file   callback_ctx.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   19.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

namespace tdsl {

    /**
     * Generic callback context
     */
    template <typename CallbackFnT>
    struct callback_ctx {
        void * user_ptr{nullptr};      // Will be passed to the callback function as first argument
        CallbackFnT callback{nullptr}; // The callback function
    };

} // namespace tdsl