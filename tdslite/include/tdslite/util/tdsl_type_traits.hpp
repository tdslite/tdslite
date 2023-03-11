/**
 * ____________________________________________________
 * Common type traits needed by the library
 *
 * These type traits are exactly same with the traditional
 * C++ standard library implementation. Some of the platforms
 * (e.g. Arduino) does not include the full C++ standard library
 * so these are mostly here for cross-platform compatibility.
 *
 * @file   tds_type_traits.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier: MIT
 * ____________________________________________________
 */

#ifndef TDSL_UTIL_TYPE_TRAITS_HPP
#define TDSL_UTIL_TYPE_TRAITS_HPP

namespace tdsl { namespace traits {

    // --------------------------------------------------------------------------------
    // enable_if

    template <bool, typename T = void>
    struct enable_if;

    template <typename T>
    struct enable_if<true, T> {
        using type = T;
    };

    // --------------------------------------------------------------------------------
    // dependent_bool

    template <bool T>
    struct dependent_bool {
        static constexpr bool value = T;
    };

    // --------------------------------------------------------------------------------
    // conditional

    template <bool B, class T, class F>
    struct conditional {
        using type = T;
    };

    template <class T, class F>
    struct conditional<false, T, F> {
        using type = F;
    };

    // --------------------------------------------------------------------------------
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

    // --------------------------------------------------------------------------------
    // remove_cv

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

    // --------------------------------------------------------------------------------
    // remove_cvref

    template <typename T>
    struct remove_cvref {
        using type = typename remove_reference<typename remove_cv<T>::type>::type;
    };

    // --------------------------------------------------------------------------------
    // integral_constant

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

    // --------------------------------------------------------------------------------
    // true_type

    using true_type  = integral_constant<bool, true>;

    // --------------------------------------------------------------------------------
    // false_type

    using false_type = integral_constant<bool, false>;

    // --------------------------------------------------------------------------------
    // is_const

    template <class T>
    struct is_const : false_type {};

    template <class T>
    struct is_const<const T> : true_type {};

    // --------------------------------------------------------------------------------
    // is_volatile

    template <class T>
    struct is_volatile : false_type {};

    template <class T>
    struct is_volatile<volatile T> : true_type {};

    // --------------------------------------------------------------------------------
    // disjunction

    template <class...>
    struct disjunction : false_type {};

    template <class T1>
    struct disjunction<T1> : T1 {};

    template <class T1, class... Tn>
    struct disjunction<T1, Tn...> : conditional<bool(T1::value), T1, disjunction<Tn...>>::type {};

    // --------------------------------------------------------------------------------
    // op_or

    template <typename...>
    struct op_or;

    template <>
    struct op_or<> : public false_type {};

    template <typename T1>
    struct op_or<T1> : public T1 {};

    template <typename T1, typename T2>
    struct op_or<T1, T2> : public conditional<T1::value, T1, T2>::type {};

    template <typename T1, typename T2, typename T3, typename... Tn>
    struct op_or<T1, T2, T3, Tn...>
        : public conditional<T1::value, T1, op_or<T2, T3, Tn...>>::type {};

    // --------------------------------------------------------------------------------
    // is_same

    template <typename T, typename U>
    struct is_same : false_type {};

    template <typename T>
    struct is_same<T, T> : true_type {};

    // --------------------------------------------------------------------------------
    // is_lvalue_reference

    template <typename>
    struct is_lvalue_reference : public false_type {};

    template <typename T>
    struct is_lvalue_reference<T &> : public true_type {};

    // --------------------------------------------------------------------------------
    // is_rvalue_reference

    template <typename>
    struct is_rvalue_reference : public false_type {};

    template <typename T>
    struct is_rvalue_reference<T &&> : public true_type {};

    // --------------------------------------------------------------------------------
    // is_reference

    template <typename T>
    struct is_reference : public op_or<is_lvalue_reference<T>, is_rvalue_reference<T>>::type {};

    // --------------------------------------------------------------------------------
    // is_function

    template <class T>
    struct is_function
        : integral_constant<bool, !is_const<const T>::value && !is_reference<T>::value> {};

    // --------------------------------------------------------------------------------
    // detail::is_integral_base

    namespace detail {
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
    } // namespace detail

    // --------------------------------------------------------------------------------
    // is_integral

    template <typename T>
    struct is_integral : detail::is_integral_base<typename remove_cv<T>::type> {};

    // --------------------------------------------------------------------------------
    // is_floating_point

    template <class T>
    struct is_floating_point
        : integral_constant<bool, is_same<float, typename remove_cv<T>::type>::value ||
                                      is_same<double, typename remove_cv<T>::type>::value ||
                                      is_same<long double, typename remove_cv<T>::type>::value> {};

    // --------------------------------------------------------------------------------
    // is_arithmetic

    template <typename T>
    struct is_arithmetic
        : integral_constant<bool, is_integral<T>::value || is_floating_point<T>::value> {};

    // --------------------------------------------------------------------------------
    // detail::is_void_helper

    namespace detail {
        template <typename>
        struct is_void_helper : public false_type {};

        template <>
        struct is_void_helper<void> : public true_type {};
    } // namespace detail

    // --------------------------------------------------------------------------------
    // is_void

    /// is_void
    template <typename T>
    struct is_void : public detail::is_void_helper<typename remove_cv<T>::type>::type {};

    // --------------------------------------------------------------------------------
    // make_void

    template <typename... Ts>
    struct make_void {
        typedef void type;
    };

    // --------------------------------------------------------------------------------
    // void_t

    template <typename... Ts>
    using void_t = typename make_void<Ts...>::type;

    // --------------------------------------------------------------------------------
    // detail::declval_impl

    namespace detail {
        template <class T>
        T && declval_impl(int);
        template <class T>
        T declval_impl(long);
    } // namespace detail

    // --------------------------------------------------------------------------------
    // declval

    template <class T>
    decltype(detail::declval_impl<T>(0)) declval() noexcept;

    // --------------------------------------------------------------------------------
    // detail::detector

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

    // --------------------------------------------------------------------------------
    // nonesuch

    struct nonesuch {
        ~nonesuch()                      = delete;
        nonesuch(const nonesuch &)       = delete;
        void operator=(const nonesuch &) = delete;
    };

    // --------------------------------------------------------------------------------
    // is_detected

    template <template <class...> class Op, class... Args>
    using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

    // --------------------------------------------------------------------------------
    // detected_t

    template <template <class...> class Op, class... Args>
    using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

    // --------------------------------------------------------------------------------
    // detected_or

    template <class Default, template <class...> class Op, class... Args>
    using detected_or = detail::detector<Default, void, Op, Args...>;

    // --------------------------------------------------------------------------------
    // is_referenceable

    template <typename T, typename = void>
    struct is_referenceable : public false_type {};

    template <typename T>
    struct is_referenceable<T, void_t<T &>> : public true_type {};

    // --------------------------------------------------------------------------------
    // add_lvalue_reference

    template <typename T, bool = is_referenceable<T>::value>
    struct add_lvalue_reference {
        using type = T;
    };

    template <typename T>
    struct add_lvalue_reference<T, true> {
        using type = T &;
    };

    // --------------------------------------------------------------------------------
    // add_rvalue_reference

    template <typename T, bool = is_referenceable<T>::value>
    struct add_rvalue_reference {
        using type = T;
    };

    template <typename T>
    struct add_rvalue_reference<T, true> {
        using type = T &&;
    };

    // --------------------------------------------------------------------------------
    // add_pointer

    template <typename T, bool = op_or<is_referenceable<T>, is_void<T>>::value>
    struct add_pointer {
        using type = T;
    };

    template <typename T>
    struct add_pointer<T, true> {
        using type = typename remove_reference<T>::type *;
    };

    // --------------------------------------------------------------------------------
    // add_const

    template <typename T>
    struct add_const {
        using type = const T;
    };

    // --------------------------------------------------------------------------------
    // add_volatile

    template <typename T>
    struct add_volatile {
        using type = volatile T;
    };

    // --------------------------------------------------------------------------------
    // detail::is_class_test

    namespace detail {

        // FIXME: This check should include integral_constant<bool, !is_union>
        // but the implementation of is_union requires compiler intrinsics
        // so we're cutting corners here.
        template <class T>
        true_type is_class_test(int T::*);

        template <class>
        false_type is_class_test(...);
    } // namespace detail

    // --------------------------------------------------------------------------------
    // is_class

    template <class T>
    struct is_class : decltype(detail::is_class_test<T>(nullptr)) {};

    // --------------------------------------------------------------------------------
    // detail::test_pre_is_base_of

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

    // --------------------------------------------------------------------------------
    // is_base_of

    template <typename Base, typename Derived>
    struct is_base_of
        : integral_constant<bool,
                            is_class<Base>::value &&
                                is_class<Derived>::value && decltype(detail::test_pre_is_base_of<
                                                                     Base, Derived>(0))::value> {};

    // --------------------------------------------------------------------------------
    // is_array

    template <class T>
    struct is_array : false_type {};

    template <class T>
    struct is_array<T []> : true_type {};

    template <class T, unsigned long N>
    struct is_array<T [N]> : true_type {};

    // --------------------------------------------------------------------------------
    // remove_extent

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

    // --------------------------------------------------------------------------------
    // decay

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

    // --------------------------------------------------------------------------------
    // detail::is_template_instance_of_impl

    namespace {
        template <typename, template <typename...> class>
        struct is_template_instance_of_impl : public false_type {};

        template <template <typename...> class U, typename... Ts>
        struct is_template_instance_of_impl<U<Ts...>, U> : public true_type {};
    } // namespace

    // --------------------------------------------------------------------------------
    // is_template_instance_of

    template <typename T, template <typename...> class U>
    using is_template_instance_of = is_template_instance_of_impl<typename decay<T>::type, U>;

    // --------------------------------------------------------------------------------

    namespace enable_when {

        // --------------------------------------------------------------------------------
        // enable_when::template_instance_of

        template <typename T, template <typename...> class U>
        using template_instance_of =
            typename traits::enable_if<traits::is_template_instance_of<T, U>::value, bool>::type;

        // --------------------------------------------------------------------------------
        // enable_when::integral

        template <typename T>
        using integral = typename enable_if<is_integral<T>::value, bool>::type;

        // --------------------------------------------------------------------------------
        // enable_when::arithmetic

        template <typename T>
        using arithmetic = typename enable_if<is_arithmetic<T>::value, bool>::type;

        // --------------------------------------------------------------------------------
        // enable_when::same

        template <typename T, typename Q>
        using same = typename enable_if<is_same<T, Q>::value, bool>::type;

        // --------------------------------------------------------------------------------
        // enable_when::non_const

        template <typename T>
        using not_const = typename enable_if<!is_const<T>::value, bool>::type;

        // --------------------------------------------------------------------------------
        // enable_when::same_any_of

        /**
         * Enable if the given type T is same with any of the types listed in type list Q
         *
         * @tparam T The type
         * @tparam Q The type list
         */
        template <typename T, typename... Q>
        using same_any_of = typename enable_if<disjunction<is_same<T, Q>...>::value, bool>::type;

        // --------------------------------------------------------------------------------
        // enable_when::not_same

        template <typename T, typename Q>
        using not_same = typename enable_if<!is_same<T, Q>::value, bool>::type;

        // --------------------------------------------------------------------------------
        // enable_when::class_type

        template <typename T>
        using class_type = typename enable_if<is_class<T>::value, bool>::type;

        // --------------------------------------------------------------------------------
        // enable_when::non_class_type

        template <typename T>
        using non_class_type = typename enable_if<!is_class<T>::value, bool>::type;

        // --------------------------------------------------------------------------------
        // enable_when::base_of

        template <typename T, typename Q>
        using base_of = typename enable_if<is_base_of<T, Q>::value, bool>::type;

    } // namespace enable_when

    // --------------------------------------------------------------------------------
    // detail::make_signed_helper

    namespace detail {
        template <typename T>
        struct make_signed_helper; // undefined for all types by default

        template <>
        struct make_signed_helper<signed char> {
            using type = signed char;
        };

        template <>
        struct make_signed_helper<signed short> {
            using type = signed short;
        };

        template <>
        struct make_signed_helper<signed int> {
            using type = signed int;
        };

        template <>
        struct make_signed_helper<signed long> {
            using type = signed long;
        };

        template <>
        struct make_signed_helper<signed long long> {
            using type = signed long long;
        };

        template <>
        struct make_signed_helper<char> {
            using type = signed char;
        };

        template <>
        struct make_signed_helper<unsigned char> {
            using type = signed char;
        };

        template <>
        struct make_signed_helper<unsigned short> {
            using type = signed short;
        };

        template <>
        struct make_signed_helper<unsigned int> {
            using type = signed int;
        };

        template <>
        struct make_signed_helper<unsigned long> {
            using type = signed long;
        };

        template <>
        struct make_signed_helper<unsigned long long> {
            using type = signed long long;
        };

        // --------------------------------------------------------------------------------
        // detail::make_unsigned_helper

        template <typename T>
        struct make_unsigned_helper; // undefined for all types by default

        template <>
        struct make_unsigned_helper<signed char> {
            using type = unsigned char;
        };

        template <>
        struct make_unsigned_helper<signed short> {
            using type = unsigned short;
        };

        template <>
        struct make_unsigned_helper<signed int> {
            using type = unsigned int;
        };

        template <>
        struct make_unsigned_helper<signed long> {
            using type = unsigned long;
        };

        template <>
        struct make_unsigned_helper<signed long long> {
            using type = unsigned long long;
        };

        template <>
        struct make_unsigned_helper<char> {
            using type = unsigned char;
        };

        template <>
        struct make_unsigned_helper<unsigned char> {
            using type = unsigned char;
        };

        template <>
        struct make_unsigned_helper<unsigned short> {
            using type = unsigned short;
        };

        template <>
        struct make_unsigned_helper<unsigned int> {
            using type = unsigned int;
        };

        template <>
        struct make_unsigned_helper<unsigned long> {
            using type = unsigned long;
        };

        template <>
        struct make_unsigned_helper<unsigned long long> {
            using type = unsigned long long;
        };

    } // namespace detail

    // --------------------------------------------------------------------------------
    // copy_qualifiers

    template <typename SRC, typename DST>
    struct copy_qualifiers {
        using type = DST;
    };

    template <typename SRC, typename DST>
    struct copy_qualifiers<const SRC, DST> {
        using type = typename add_const<DST>::type;
    };

    template <typename SRC, typename DST>
    struct copy_qualifiers<volatile SRC, DST> {
        using type = typename add_volatile<DST>::type;
    };

    template <typename SRC, typename DST>
    struct copy_qualifiers<const volatile SRC, DST> {
        using type = typename add_volatile<typename add_const<DST>::type>::type;
    };

    static_assert(is_same<copy_qualifiers<int, char>::type, char>::value, "");
    static_assert(is_same<copy_qualifiers<const int, char>::type, const char>::value, "");
    static_assert(is_same<copy_qualifiers<volatile int, char>::type, volatile char>::value, "");
    static_assert(
        is_same<copy_qualifiers<const volatile int, char>::type, const volatile char>::value, "");

    // --------------------------------------------------------------------------------
    // make_signed

    template <typename T>
    struct make_signed {
    private:
        using naked_t        = typename remove_cv<T>::type;
        using naked_signed_t = typename detail::make_signed_helper<naked_t>::type;

    public:
        using type = typename copy_qualifiers<T, naked_signed_t>::type;
    };

    static_assert(is_same<make_signed<char>::type, signed char>::value, "");
    static_assert(is_same<make_signed<signed char>::type, signed char>::value, "");
    static_assert(is_same<make_signed<unsigned char>::type, signed char>::value, "");

    static_assert(is_same<make_signed<int>::type, signed int>::value, "");
    static_assert(is_same<make_signed<signed int>::type, signed int>::value, "");
    static_assert(is_same<make_signed<unsigned int>::type, signed int>::value, "");

    static_assert(is_same<make_signed<long>::type, signed long>::value, "");
    static_assert(is_same<make_signed<signed long>::type, signed long>::value, "");
    static_assert(is_same<make_signed<unsigned long>::type, signed long>::value, "");

    static_assert(is_same<make_signed<long long>::type, signed long long>::value, "");
    static_assert(is_same<make_signed<signed long long>::type, signed long long>::value, "");
    static_assert(is_same<make_signed<unsigned long long>::type, signed long long>::value, "");

    // --------------------------------------------------------------------------------
    // make_unsigned

    template <typename T>
    struct make_unsigned {
    private:
        using naked_t          = typename remove_cv<T>::type;
        using naked_unsigned_t = typename detail::make_unsigned_helper<naked_t>::type;

    public:
        using type = typename copy_qualifiers<T, naked_unsigned_t>::type;
    };

    static_assert(is_same<make_unsigned<char>::type, unsigned char>::value, "");
    static_assert(is_same<make_unsigned<signed char>::type, unsigned char>::value, "");
    static_assert(is_same<make_unsigned<unsigned char>::type, unsigned char>::value, "");

    static_assert(is_same<make_unsigned<int>::type, unsigned int>::value, "");
    static_assert(is_same<make_unsigned<signed int>::type, unsigned int>::value, "");
    static_assert(is_same<make_unsigned<unsigned int>::type, unsigned int>::value, "");

    static_assert(is_same<make_unsigned<long>::type, unsigned long>::value, "");
    static_assert(is_same<make_unsigned<signed long>::type, unsigned long>::value, "");
    static_assert(is_same<make_unsigned<unsigned long>::type, unsigned long>::value, "");

    static_assert(is_same<make_unsigned<long long>::type, unsigned long long>::value, "");
    static_assert(is_same<make_unsigned<signed long long>::type, unsigned long long>::value, "");
    static_assert(is_same<make_unsigned<unsigned long long>::type, unsigned long long>::value, "");

    // --------------------------------------------------------------------------------
    // detail::is_signed

    namespace detail {
        template <typename T, bool = traits::is_arithmetic<T>::value>
        struct is_signed_impl : integral_constant<bool, T(-1) < T(0)> {};

        template <typename T>
        struct is_signed_impl<T, false> : false_type {};
    } // namespace detail

    // --------------------------------------------------------------------------------
    // is_signed

    template <typename T>
    struct is_signed : detail::is_signed_impl<T>::type {};

    // --------------------------------------------------------------------------------

}} // namespace tdsl::traits

#endif