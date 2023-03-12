/**
 * _____________________________________________________
 * Default type for tdslite callbacks
 *
 * @file   tdsl_callback.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   19.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _____________________________________________________
 */

#ifndef TDSL_DETAIL_CALLBACK_HPP
#define TDSL_DETAIL_CALLBACK_HPP

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

    /**
     * Generic callback type
     *
     * @tparam T Default arg for callback function type
     * @tparam FNTYPE Callback function type
     */
    template <typename T, typename FNTYPE>
    struct callback {
        using function_type    = FNTYPE;
        using return_type      = typename function_signature<FNTYPE>::return_type;

        // --------------------------------------------------------------------------------

        /**
         * Pointer to the  callback function
         */
        FNTYPE callback_fn_ptr = {nullptr};

        // --------------------------------------------------------------------------------

        /**
         * User-supplied opaque pointer.
         * Will be passed as first argument to the callback function.
         */
        void * user_ptr        = {nullptr};

        // --------------------------------------------------------------------------------

        callback() noexcept    = default;

        // --------------------------------------------------------------------------------

        /**
         * Construct callback
         *
         * @param [in] callback Callback function
         * @param [in] user_ptr User pointer
         *
         * The @p user_ptr will be passed as first argument to
         * @p callback on every invocation.
         */
        callback(FNTYPE callback, void * user_ptr) noexcept :
            callback_fn_ptr(callback), user_ptr(user_ptr){};

        // --------------------------------------------------------------------------------

        /**
         * Operator bool to make callback boolable.
         * Returns true if callback has a non-null context
         */
        inline TDSL_NODISCARD operator bool() const noexcept {
            return callback_fn_ptr != nullptr;
        }

        // --------------------------------------------------------------------------------

        /**
         * Invoke the @ref callback with given @p args, if set.
         * The callback function will be called with @ref user_ptr
         * followed by the @p args. The call will have no effect if
         * @ref callback is nullptr.
         *
         * @param [in] args Arguments to call the callback funtion with
         * @return The return value of callback function
         */
        template <typename R = return_type, typename... Args>
        inline TDSL_NODISCARD auto operator()(Args &&... args) const noexcept ->
            typename traits::enable_if<!traits::is_same<R, void>::value, R>::type {
            return (operator bool() ? callback_fn_ptr(user_ptr, TDSL_FORWARD(args)...)
                                    : return_type{});
        }

        // --------------------------------------------------------------------------------

        /**
         * Invoke the @ref callback with given @p args, if set.
         * The callback function will be called with @ref user_ptr
         * followed by the @p args. The call will have no effect if
         * @ref callback is nullptr.
         *
         * (void return type overload)
         *
         * @param [in] args Arguments to call the callback funtion with
         */
        template <typename R = return_type, typename... Args>
        inline auto operator()(Args &&... args) const noexcept ->
            typename traits::enable_if<traits::is_same<R, void>::value, void>::type {
            if (operator bool()) {
                callback_fn_ptr(user_ptr, TDSL_FORWARD(args)...);
            }
        }
    };

    // --------------------------------------------------------------------------------

    /**
     * Default template for callback context
     */
    template <typename T, typename FNTYPE = void (*)(/*user_ptr*/ void *, /*info type*/ const T &)>
    struct callback;

} // namespace tdsl

#endif