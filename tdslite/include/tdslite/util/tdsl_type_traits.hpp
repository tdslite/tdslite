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

#ifndef TDSL_UTIL_TYPE_TRAITS_HPP
#define TDSL_UTIL_TYPE_TRAITS_HPP

namespace tdsl { namespace traits {

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

    template <bool T>
    struct dependent_bool {
        static constexpr bool value = T;
    };

    // Conditional

    template <bool B, class T, class F>
    struct conditional {
        using type = T;
    };

    template <class T, class F>
    struct conditional<false, T, F> {
        using type = F;
    };

    template <class T>
    struct is_const : false_type {};

    template <class T>
    struct is_const<const T> : true_type {};

    // Disjunction

    template <class...>
    struct disjunction : false_type {};

    template <class B1>
    struct disjunction<B1> : B1 {};

    template <class B1, class... Bn>
    struct disjunction<B1, Bn...> : conditional<bool(B1::value), B1, disjunction<Bn...>>::type {};

    template <typename...>
    struct op_or;

    template <>
    struct op_or<> : public false_type {};

    template <typename B1>
    struct op_or<B1> : public B1 {};

    template <typename B1, typename B2>
    struct op_or<B1, B2> : public conditional<B1::value, B1, B2>::type {};

    template <typename B1, typename B2, typename B3, typename... Bn>
    struct op_or<B1, B2, B3, Bn...>
        : public conditional<B1::value, B1, op_or<B2, B3, Bn...>>::type {};

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

    template <typename>
    struct is_lvalue_reference : public false_type {};

    template <typename T>
    struct is_lvalue_reference<T &> : public true_type {};

    /// is_rvalue_reference
    template <typename>
    struct is_rvalue_reference : public false_type {};

    template <typename T>
    struct is_rvalue_reference<T &&> : public true_type {};

    template <typename T>
    struct is_reference : public op_or<is_lvalue_reference<T>, is_rvalue_reference<T>>::type {};

    template <class T>
    struct is_function
        : integral_constant<bool, !is_const<const T>::value && !is_reference<T>::value> {};

    template <typename T>
    using enable_if_function = typename enable_if<is_function<T>::value, bool>::type;

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
    struct is_integral_base : public false_type {};

    template <>
    struct is_integral_base<bool> : public true_type {};

    template <>
    struct is_integral_base<char> : public true_type {};

    template <>
    struct is_integral_base<signed char> : public true_type {};

    template <>
    struct is_integral_base<unsigned char> : public true_type {};

    template <>
    struct is_integral_base<short> : public true_type {};

    template <>
    struct is_integral_base<unsigned short> : public true_type {};

    template <>
    struct is_integral_base<int> : public true_type {};

    template <>
    struct is_integral_base<unsigned int> : public true_type {};

    template <>
    struct is_integral_base<long> : public true_type {};

    template <>
    struct is_integral_base<unsigned long> : public true_type {};

    template <>
    struct is_integral_base<long long> : public true_type {};

    template <>
    struct is_integral_base<unsigned long long> : public true_type {};

    template <>
    struct is_integral_base<char16_t> : public true_type {};

    template <>
    struct is_integral_base<char32_t> : public true_type {};

    template <typename T>
    struct is_integral : is_integral_base<typename remove_cv<T>::type> {};

    namespace detail {
        template <typename>
        struct is_void_helper : public false_type {};

        template <>
        struct is_void_helper<void> : public true_type {};
    } // namespace detail

    /// is_void
    template <typename T>
    struct is_void : public detail::is_void_helper<typename remove_cv<T>::type>::type {};

    // enable_if_integral

    // void_t
    template <class...>
    using void_t = void;

    // declval

    namespace detail {
        template <class T>
        T && declval_impl(int);
        template <class T>
        T declval_impl(long);
    } // namespace detail

    template <class T>
    decltype(detail::declval_impl<T>(0)) declval() noexcept;

    //

    namespace detail {
        template <class Default, class AlwaysVoid, template <class...> class Op, class... Args>
        struct detector {
            using value_t = false_type;
            using type    = Default;
        };

        template <class Default, template <class...> class Op, class... Args>
        struct detector<Default, void_t<Op<Args...>>, Op, Args...> {
            using value_t = true_type;
            using type    = Op<Args...>;
        };

    } // namespace detail

    struct nonesuch {
        ~nonesuch()                      = delete;
        nonesuch(nonesuch const &)       = delete;
        void operator=(nonesuch const &) = delete;
    };

    template <template <class...> class Op, class... Args>
    using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

    template <template <class...> class Op, class... Args>
    using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

    template <class Default, template <class...> class Op, class... Args>
    using detected_or = detail::detector<Default, void, Op, Args...>;

    // referenceable

    template <typename T, typename = void>
    struct is_referenceable : public false_type {};

    template <typename T>
    struct is_referenceable<T, void_t<T &>> : public true_type {};

    // add reference
    template <typename T, bool = is_referenceable<T>::value>
    struct add_lvalue_reference {
        using type = T;
    };

    template <typename T>
    struct add_lvalue_reference<T, true> {
        using type = T &;
    };

    template <typename T, bool = is_referenceable<T>::value>
    struct add_rvalue_reference {
        using type = T;
    };

    template <typename T>
    struct add_rvalue_reference<T, true> {
        using type = T &&;
    };

    template <typename T, bool = op_or<is_referenceable<T>, is_void<T>>::value>
    struct add_pointer {
        using type = T;
    };

    template <typename T>
    struct add_pointer<T, true> {
        using type = typename remove_reference<T>::type *;
    };

    namespace detail {

        // FIXME: This check should include integral_constant<bool, !is_union>
        // but the implementation of is_union requires compiler intrinsics
        // so we're cutting corners here.
        template <class T>
        true_type is_class_test(int T::*);

        template <class>
        false_type is_class_test(...);
    } // namespace detail

    template <class T>
    struct is_class : decltype(detail::is_class_test<T>(nullptr)) {};

    // is_base_of detail
    namespace detail {
        template <typename B>
        true_type test_pre_ptr_convertible(const volatile B *);
        template <typename>
        false_type test_pre_ptr_convertible(const volatile void *);

        template <typename, typename>
        auto test_pre_is_base_of(...) -> true_type;
        template <typename B, typename D>
        auto test_pre_is_base_of(int)
            -> decltype(test_pre_ptr_convertible<B>(static_cast<D *>(nullptr)));
    } // namespace detail

    template <typename Base, typename Derived>
    struct is_base_of
        : integral_constant<bool,
                            is_class<Base>::value &&
                                is_class<Derived>::value && decltype(detail::test_pre_is_base_of<
                                                                     Base, Derived>(0))::value> {};

    // array

    template <class T>
    struct is_array : false_type {};

    template <class T>
    struct is_array<T []> : true_type {};

    template <class T, unsigned long N>
    struct is_array<T [N]> : true_type {};

    template <class T>
    struct remove_extent {
        typedef T type;
    };

    template <class T>
    struct remove_extent<T []> {
        typedef T type;
    };

    template <class T, unsigned long N>
    struct remove_extent<T [N]> {
        typedef T type;
    };

    template <class T>
    struct decay {
    private:
        typedef typename remove_reference<T>::type U;

    public:
        typedef typename conditional<
            is_array<U>::value, typename remove_extent<U>::type *,
            typename conditional<is_function<U>::value, typename add_pointer<U>::type,
                                 typename remove_cv<U>::type>::type>::type type;
    };

    namespace {
        template <typename, template <typename...> class>
        struct is_template_instance_of_impl : public false_type {};

        template <template <typename...> class U, typename... Ts>
        struct is_template_instance_of_impl<U<Ts...>, U> : public true_type {};
    } // namespace

    template <typename T, template <typename...> class U>
    using is_template_instance_of = is_template_instance_of_impl<typename decay<T>::type, U>;

    namespace enable_when {
        template <typename T, template <typename...> class U>
        using template_instance_of =
            typename traits::enable_if<traits::is_template_instance_of<T, U>::value, bool>::type;

        template <typename T>
        using integral = typename enable_if<is_integral<T>::value, bool>::type;

        template <typename T, typename Q>
        using same = typename enable_if<is_same<T, Q>::value, bool>::type;

        /**
         * Enable if the given type T is same with any of the types listed in type list Q
         *
         * @tparam T The type
         * @tparam Q The type list
         */
        template <typename T, typename... Q>
        using same_any_of = typename enable_if<disjunction<is_same<T, Q>...>::value, bool>::type;

        template <typename T, typename Q>
        using not_same = typename enable_if<!is_same<T, Q>::value, bool>::type;

        template <typename T>
        using class_type = typename enable_if<is_class<T>::value, bool>::type;

        template <typename T>
        using non_class_type = typename enable_if<!is_class<T>::value, bool>::type;

    } // namespace enable_when

}} // namespace tdsl::traits

#endif