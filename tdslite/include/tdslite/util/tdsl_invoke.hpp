/**
 * _________________________________________________
 *
 * @file   tdsl_invoke.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   19.09.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSL_UTIL_TDS_INVOKE_HPP
#define TDSL_UTIL_TDS_INVOKE_HPP

#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>

namespace tdsl {
    namespace detail {
        template <class>
        constexpr bool is_reference_wrapper_v = false;
        template <class U>
        constexpr bool is_reference_wrapper_v<std::reference_wrapper<U>> = true;

        template <class C, class Pointed, class T1, class... Args>
        constexpr decltype(auto) invoke_memptr(Pointed C::*f, T1 && t1, Args &&... args) {
            if constexpr (traits::is_function<Pointed>::value) {
                if constexpr (traits::is_base_of<C, typename traits::decay<T1>::type>::value)
                    return (TDSL_FORWARD(t1).*f)(TDSL_FORWARD(args)...);
                else if constexpr (is_reference_wrapper_v<std::decay_t<T1>>)
                    return (t1.get().*f)(TDSL_FORWARD(args)...);
                else
                    return ((*std::forward<T1>(t1)).*f)(std::forward<Args>(args)...);
            }
            else {
                static_assert(std::is_object_v<Pointed> && sizeof...(args) == 0);
                if constexpr (std::is_base_of_v<C, std::decay_t<T1>>)
                    return std::forward<T1>(t1).*f;
                else if constexpr (is_reference_wrapper_v<std::decay_t<T1>>)
                    return t1.get().*f;
                else
                    return (*std::forward<T1>(t1)).*f;
            }
        }

        template <typename Q = T, traits::enable_if_function<Pointed> = true>
        static void construct(Q * storage, tdsl::uint32_t n_elems) {
            for (tdsl::uint32_t i = 0; i < n_elems; i++) {
                new (storage + i) Q();
            }
        }

        // template<class C, class P, class T1, class... Args>

    } // namespace detail

    // since we're targeting C++11, let's
    // SFINAE the s**t out of this:

    // template <class F, class... Args>
    // constexpr std::invoke_result_t<F, Args...> invoke(F && f, Args &&... args)
    // noexcept(std::is_nothrow_invocable_v<F, Args...>) {
    //     if constexpr (std::is_member_pointer_v<std::decay_t<F>>)
    //         return detail::invoke_memptr(f, std::forward<Args>(args)...);
    //     else
    //         return std::forward<F>(f)(std::forward<Args>(args)...);
    // }

} // namespace tdsl

#endif