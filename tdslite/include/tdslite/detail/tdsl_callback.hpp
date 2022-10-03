/**
 * _____________________________________________________
 * Default type for tdslite callbacks
 *
 * @file   tdsl_callback.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   19.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _____________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>

namespace tdsl {

    // --------------------------------------------------------------------------------

    template <typename FNTYPE>
    struct function_signature;

    template <typename R, typename... Args>
    struct function_signature<R (*)(Args...)> {
        using type        = R (*)(Args...);
        using return_type = R;
    };

    template <typename T, typename FNTYPE>
    struct callback {
        using function_type = FNTYPE;
        using return_type   = typename function_signature<FNTYPE>::return_type;

        /**
         * User-supplied opaque pointer.
         * Will be passed as first argument to the callback function.
         */
        void * user_ptr{nullptr};

        /**
         * Pointer to the  callback function
         */
        FNTYPE callback{nullptr};

        // --------------------------------------------------------------------------------

        /**
         * Set callback context
         *
         * @param user_ptr User pointer
         * @param callback Callback function
         *
         * The @p user_ptr will be passed as first argument to @p callback
         * on invocation.
         */
        void set(void * user_ptr, FNTYPE callback) {
            this->user_ptr = user_ptr;
            this->callback = callback;
        }

        // --------------------------------------------------------------------------------

        /**
         * Operator bool to make callback boolable.
         * Returns true if callback has a non-null context
         */
        inline operator bool() const noexcept {
            return callback != nullptr;
        }

        // --------------------------------------------------------------------------------

        /**
         * Invoke the @ref callback with given @p args. The callback function
         * will be called with @ref user_ptr followed by the @p args.
         *
         * @param [in] args Arguments to call the callback funtion with
         * @return The return value of callback function
         */
        template <typename R = return_type, typename... Args>
        inline auto maybe_invoke(Args &&... args) -> typename traits::enable_if<!traits::is_same<R, void>::value, R>::type {
            return (operator bool() ? invoke(TDSL_FORWARD(args)...) : return_type{0});
        }

        // --------------------------------------------------------------------------------

        /**
         * Invoke the @ref callback with given @p args. The callback function
         * will be called with @ref user_ptr followed by the @p args.
         *
         * (void return type overload)
         *
         * @param [in] args Arguments to call the callback funtion with
         */
        template <typename R = return_type, typename... Args>
        inline auto maybe_invoke(Args &&... args) -> typename traits::enable_if<traits::is_same<R, void>::value, void>::type {
            if (operator bool()) {
                invoke(TDSL_FORWARD(args)...);
            }
        }

        // --------------------------------------------------------------------------------

        /**
         * Invoke the @ref callback with given @p args. The callback function
         * will be called with @ref user_ptr followed by the @p args.
         *
         * @param [in] args Arguments to call the callback funtion with
         * @return The return value of callback function
         */
        template <typename R = return_type, typename... Args>
        inline auto invoke(Args &&... args) -> typename traits::enable_if<!traits::is_same<R, void>::value, R>::type {
            return callback(user_ptr, TDSL_FORWARD(args)...);
        }

        // --------------------------------------------------------------------------------

        template <typename R = return_type, typename... Args>
        inline auto invoke(Args &&... args) -> typename traits::enable_if<traits::is_same<R, void>::value, void>::type {
            callback(user_ptr, TDSL_FORWARD(args)...);
        }
    };

    /**
     * Default template for callback context
     */
    template <typename T, typename FNTYPE = tdsl::uint32_t (*)(/*user_ptr*/ void *, /*info type*/ const T &)>
    struct callback;

} // namespace tdsl