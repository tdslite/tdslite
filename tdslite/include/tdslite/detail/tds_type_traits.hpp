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

namespace tdslite { namespace traits {

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

    // enable_if
    template <bool, typename T = void>
    struct enable_if;

    template <typename T>
    struct enable_if<true, T> {
        using type = T;
    };

    // is_same
    template <typename T, typename U>
    struct is_same : false_type {};
    template <typename T>
    struct is_same<T, T> : true_type {};

    // remove_reference
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

    /// remove_cv
    template <typename T>
    struct remove_cv {
        using type = T;
    };

    template <typename T>
    struct remove_cv<const T> {
        using type = T;
    };

    template <typename T>
    struct remove_cv<volatile T> {
        using type = T;
    };

    template <typename T>
    struct remove_cv<const volatile T> {
        using type = T;
    };

    // is_integral
    template <typename>
    struct is_integral : public false_type {};

    template <>
    struct is_integral<bool> : public true_type {};

    template <>
    struct is_integral<char> : public true_type {};

    template <>
    struct is_integral<signed char> : public true_type {};

    template <>
    struct is_integral<unsigned char> : public true_type {};

    template <>
    struct is_integral<short> : public true_type {};

    template <>
    struct is_integral<unsigned short> : public true_type {};

    template <>
    struct is_integral<int> : public true_type {};

    template <>
    struct is_integral<unsigned int> : public true_type {};

    template <>
    struct is_integral<long> : public true_type {};

    template <>
    struct is_integral<unsigned long> : public true_type {};

    template <>
    struct is_integral<long long> : public true_type {};

    template <>
    struct is_integral<unsigned long long> : public true_type {};

    template <>
    struct is_integral<char16_t> : public true_type {};

    template <>
    struct is_integral<char32_t> : public true_type {};

    // enable_if_integral

    template <typename T>
    using enable_if_integral = typename enable_if<is_integral<T>::value, bool>::type;

    template <typename T, typename Q>
    using enable_if_same = typename enable_if<is_same<T, Q>::value, bool>::type;

    template <typename T, typename Q>
    using enable_if_not_same = typename enable_if<!is_same<T, Q>::value, bool>::type;

}} // namespace tdslite::traits

#endif