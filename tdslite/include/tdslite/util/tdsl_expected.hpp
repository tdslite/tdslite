/**
 * _________________________________________________
 *
 * @file   tdsl_expected.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   16.09.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>

namespace tdsl {

    // ST -> (in particular, array and reference types are not allowed
    // ET -> in particular, arrays, non-object types, and cv-qualified types are not allowed

    // TODO: limit this to arithmetic & enum types
    // so we don't have to deal with non-triviality
    // of others.
    template <typename ET>
    struct unexpected {
        unexpected() = default;

        unexpected(ET && unexpected) : value(TDSLITE_MOVE(unexpected)) {}

        ET value;
        static_assert(tdsl::traits::is_reference<ET>::value == false, "ET cannot be a reference type");
    };

    template <typename ET>
    inline auto make_unexpected(ET v) -> unexpected<ET> {
        return unexpected<ET>(v);
    }

    /**
     * Barebones* expected implementation
     * (*not exactly standard compliant)
     *
     * @tparam ST Success data type
     * @tparam ET Error data type
     */
    template <typename ST, typename ET = tdsl::uint32_t>
    struct expected {
    private:
        static_assert(tdsl::traits::is_reference<ST>::value == false, "ST cannot be a reference type");

        // member access wrapper
        struct maw {
            constexpr explicit maw(ST & value) : value(value) {}

            constexpr auto operator->() const noexcept -> ST * {
                return &value;
            }

        private:
            ST & value;
        };

    public:
        using unexpected_type = unexpected<ET>;

        union {
            ST value;
            unexpected_type unexpected_value{};
        };

        // TODO operator *
        // TODO operator ->

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
         * @param expected
         */
        expected(ST && expected) : value(TDSLITE_MOVE(expected)), has_expected(true) {}

        expected(expected<ST, ET> && other) {
            if (other.has_value()) {
                value        = TDSLITE_MOVE(other.value);
                has_expected = true;
            }
            else {
                unexpected_value = TDSLITE_MOVE(unexpected_value);
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
        expected(unexpected_type unexpected) : unexpected_value(unexpected), has_expected(false) {}

        inline ~expected() {
            // ... because destructor cannot be a templated function.
            destructor_impl();
        }

        // By default, expected contains ET.
        bool has_expected = {false};

        inline bool has_value() const noexcept {
            return has_expected;
        }

        inline operator bool() const noexcept {
            return has_value();
        }

        inline ST & get() & {
            TDSLITE_ASSERT_MSG(has_value(), "invalid expected access,expected does not have a value");
            return value;
        }

        inline const ST & get() const & {
            TDSLITE_ASSERT_MSG(has_value(), "invalid expected access,expected does not have a value");
            return value;
        }

        inline ST && get() && {
            TDSLITE_ASSERT_MSG(has_value(), "invalid expected access,expected does not have a value");
            return TDSLITE_MOVE(value);
        }

        inline const ST && get() const && {
            TDSLITE_ASSERT_MSG(has_value(), "invalid expected access,expected does not have a value");
            return TDSLITE_MOVE(value);
        }

        inline ET & error() & {
            TDSLITE_ASSERT_MSG(!has_value(), "invalid expected access, unexpected does not have a value");
            return unexpected_value.value;
        }

        inline const ET & error() const & {
            TDSLITE_ASSERT_MSG(!has_value(), "invalid expected access, unexpected does not have a value");
            return unexpected_value.value;
        }

        inline ET && error() && {
            TDSLITE_ASSERT_MSG(!has_value(), "invalid expected access, unexpected does not have a value");
            return TDSLITE_MOVE(unexpected_value.value);
        }

        inline const ET && error() const && {
            TDSLITE_ASSERT_MSG(!has_value(), "invalid expected access, unexpected does not have a value");
            return TDSLITE_MOVE(unexpected_value.value);
        }

    private:
        template <typename Q = ST, traits::enable_if_class<Q> = true>
        void destructor_impl() {
            if (has_expected) {
                value.~ST();
            }
        }

        // noop
        template <typename Q>
        void destructor_impl() {}
    };

} // namespace tdsl