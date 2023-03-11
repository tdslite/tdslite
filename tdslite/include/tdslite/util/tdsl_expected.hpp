/**
 * ____________________________________________________
 * C++11 *just enough* *expected* implementation
 * backported from C++23
 *
 * @file   tdsl_expected.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   16.09.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_UTIL_EXPECTED_HPP
#define TDSL_UTIL_EXPECTED_HPP

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>

namespace tdsl {

    // ST -> (in particular, array and reference types are not allowed
    // ET -> in particular, arrays, non-object types, and cv-qualified types are not allowed

    // TODO: limit this to arithmetic & enum types
    // so we don't have to deal with non-triviality
    // of others.
    // template <typename ET>
    // struct unexpected {
    //     unexpected() = default;

    //     unexpected(ET && unexpected) : value(TDSL_MOVE(unexpected)) {}

    //     ET value;
    //     static_assert(tdsl::traits::is_reference<ET>::value == false,
    //                   "ET cannot be a reference type");
    // };

    /**
     * Barebones* expected implementation
     * (*not exactly standard compliant)
     *
     * @tparam ST Success data type
     * @tparam ET Error data type
     */
    template <typename ST, typename ET = tdsl::int32_t>
    struct expected {
    private:
        using self_type = expected<ST, ET>;
        static_assert(tdsl::traits::is_reference<ST>::value == false,
                      "ST cannot be a reference type");

        // member access wrapper
        struct maw {
            constexpr explicit maw(ST & value) : value(value) {}

            constexpr auto operator->() const noexcept -> ST * {
                return &value;
            }

        private:
            ST & value;
        };

        struct unexpected_ctr_tag {};

    public:
        static inline auto unexpected(ET v) noexcept -> self_type {
            return self_type{v, unexpected_ctr_tag{}};
        }

        union {
            ST value;
            ET unexpected_value{};
        };

        inline auto operator->() noexcept -> maw {
            return maw{get()};
        }

        /**
         * Dereference (star) operator overload
         *
         * @return cv-qualified reference to the resource
         */
        inline auto operator*() noexcept -> ST & {
            return get();
        }

        /**
         * Construct a new expected object
         *
         * @param [in] expected The expected value
         */
        expected(ST && expected) : value(TDSL_MOVE(expected)), has_expected(true) {}

        expected(expected<ST, ET> && other) {
            if (other.has_value()) {
                value        = TDSL_MOVE(other.value);
                has_expected = true;
            }
            else {
                unexpected_value = TDSL_MOVE(unexpected_value);
                has_expected     = false;
            }
        }

        /**
         * Expected constructor for unexpected
         *
         * Only allow calls with tag dispatch, since ST == ET and
         * it would be ambigious.
         *
         * @param [in] unexpected Value for unexpected
         */
        expected(ET unexpected, unexpected_ctr_tag) :
            unexpected_value(unexpected), has_expected(false) {}

        inline ~expected() {
            // ... because destructor cannot be a templated function.
            destructor_impl();
        }

        // By default, expected contains ET.
        bool has_expected = {false};

        inline TDSL_NODISCARD bool has_value() const noexcept {
            return has_expected;
        }

        inline operator bool() const noexcept {
            return has_value();
        }

        inline TDSL_NODISCARD ST & get() & {
            TDSL_ASSERT_MSG(has_value(), "invalid expected access,expected does not have a value");
            return value;
        }

        inline TDSL_NODISCARD const ST & get() const & {
            TDSL_ASSERT_MSG(has_value(), "invalid expected access,expected does not have a value");
            return value;
        }

        inline TDSL_NODISCARD ST && get() && {
            TDSL_ASSERT_MSG(has_value(), "invalid expected access,expected does not have a value");
            return TDSL_MOVE(value);
        }

        inline TDSL_NODISCARD const ST && get() const && {
            TDSL_ASSERT_MSG(has_value(), "invalid expected access,expected does not have a value");
            return TDSL_MOVE(value);
        }

        inline TDSL_NODISCARD ET & error() & {
            TDSL_ASSERT_MSG(!has_value(),
                            "invalid expected access, unexpected does not have a value");
            return unexpected_value;
        }

        inline TDSL_NODISCARD const ET & error() const & {
            TDSL_ASSERT_MSG(!has_value(),
                            "invalid expected access, unexpected does not have a value");
            return unexpected_value;
        }

        inline TDSL_NODISCARD ET && error() && {
            TDSL_ASSERT_MSG(!has_value(),
                            "invalid expected access, unexpected does not have a value");
            return TDSL_MOVE(unexpected_value);
        }

        inline TDSL_NODISCARD const ET && error() const && {
            TDSL_ASSERT_MSG(!has_value(),
                            "invalid expected access, unexpected does not have a value");
            return TDSL_MOVE(unexpected_value);
        }

    private:
        template <typename Q = ST, traits::enable_when::class_type<Q> = true>
        void destructor_impl() {
            if (has_expected) {
                value.~ST();
            }
        }

        template <typename Q = ST, traits::enable_when::non_class_type<Q> = true>
        void destructor_impl() {}
    };

} // namespace tdsl

#endif