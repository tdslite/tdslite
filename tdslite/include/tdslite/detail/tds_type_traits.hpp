/**
 * ____________________________________________________
 * Common type traits needed by the library
 *
 * @file   tds_type_traits.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSLITE_DETAIL_TDS_TYPE_TRAITS_HPP
#define TDSLITE_DETAIL_TDS_TYPE_TRAITS_HPP

namespace tdslite { namespace detail { namespace traits {

    // These type traits are exactly same with the traditional
    // C++ standard library implementation. Some of the platforms
    // (e.g. Arduino) does not include the full C++ standard library
    // so these are mostly here for cross-platform compatibility.

    template <class T, T V>
    struct integral_constant {
        static constexpr T value = V;
        using value_type         = T;
        using type               = integral_constant;
        constexpr operator value_type() const noexcept {
            return value;
        }
        constexpr value_type operator()() const noexcept {
            return value;
        }
    };

    using true_type  = integral_constant<bool, true>;
    using false_type = integral_constant<bool, false>;

    template <bool, typename T = void>
    struct enable_if;

    template <typename T>
    struct enable_if<true, T> {
        using type = T;
    };

    template <typename T, typename U>
    struct is_same : false_type {};
    template <typename T>
    struct is_same<T, T> : true_type {};

    template <typename T>
    struct remove_reference {
        typedef T type;
    };

    template <typename T>
    struct remove_reference<T &> {
        typedef T type;
    };

    template <typename T>
    struct remove_reference<T &&> {
        typedef T type;
    };
}}} // namespace tdslite::detail::traits

#endif