/**
 * ____________________________________________________
 * FIXME: Description
 *
 * @file   tdsl_field.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TDSL_FIELD_HPP
#define TDSL_DETAIL_TDSL_FIELD_HPP

#include <tdslite/util/tdsl_expected.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/detail/tdsl_tds_column_info.hpp>
#include <tdslite/detail/sqltypes/sql_type_base.hpp>
#include <tdslite/detail/sqltypes/sql_basics.hpp>
#include <tdslite/detail/sqltypes/sql_datetime.hpp>
#include <tdslite/detail/sqltypes/sql_smalldatetime.hpp>
#include <tdslite/detail/sqltypes/sql_money.hpp>
#include <tdslite/detail/sqltypes/sql_decimal.hpp>

namespace tdsl {

    namespace detail {
        template <typename NetImpl>
        struct command_context;

        // --------------------------------------------------------------------------------

        /**
         * Cast helper for arithmetic types
         *
         * @param [in] data Data to cast
         * @return T bytes of data converted to host endianness and reinterpreted as type T
         */
        template <typename T, typename traits::enable_when::arithmetic<T> = true>
        inline auto as_impl(byte_view data, const tdsl::tds_column_info &) -> T {
            TDSL_ASSERT_MSG(data.size_bytes() >= sizeof(T),
                            "Given span does not have enough bytes to read a value with type T!");
            return tdsl::binary_reader<tdsl::endian::little>{data}.read<T>();
        }

        // --------------------------------------------------------------------------------

        template <typename T,
                  typename traits::enable_when::template_instance_of<T, tdsl::span> = true>
        inline auto as_impl(byte_view data, const tdsl::tds_column_info &) -> T {
            return data.rebind_cast<typename T::element_type>();
        }

        // --------------------------------------------------------------------------------

        template <typename T, typename traits::enable_when::base_of<sql_type_base, T> = true>
        inline auto as_impl(byte_view data, const tdsl::tds_column_info & col) -> T {
            return T{data, col};
        }

    } // namespace detail

    /**
     * Non-owning view of a row field.
     */
    struct tdsl_field : public byte_view {
        // using byte_view::span;
        using byte_view::operator=;

        template <typename... Args>
        inline explicit tdsl_field(const tdsl::tds_column_info & col, Args &&... args) noexcept :
            byte_view(TDSL_FORWARD(args)...), column(col) {}

        // --------------------------------------------------------------------------------

        // (mgilor): Little hack to make implicit
        // construction on assignment work, e.g. tdsl_field = buf[50];
        template <typename... Args>
        inline auto operator=(Args &&... args) noexcept -> tdsl_field & {
            byte_view::operator=(TDSL_FORWARD(args)...);
            return *this;
        }

        // --------------------------------------------------------------------------------

        template <typename T>
        inline TDSL_NODISCARD auto as() const noexcept -> T {
            return detail::as_impl<T>(*this, column_info());
        }

        // --------------------------------------------------------------------------------

        /**
         * Check if field is NULL
         *
         * Note that NULL is not equivalent to value with
         * zero length. In SQL world, these two are separate
         * things.
         *
         * @return true Value is NULL, false otherwise
         */
        inline bool is_null() const noexcept {
            return data() == null_sentinel();
        }

        inline const tdsl::tds_column_info & column_info() const noexcept {
            return column;
        }

    private:
        const tdsl::tds_column_info & column;

        /**
         * Set this field as NULL.
         */
        void set_null() {
            (*this) = byte_view(null_sentinel(), null_sentinel());
            TDSL_ASSERT(data() == null_sentinel());
            TDSL_ASSERT(size() == 0);
        }

        // --------------------------------------------------------------------------------

        /**
         * Data value that represents a NULL field.
         * This is the discrimination between an empty string
         * and a NULL string.
         */
        inline const tdsl::uint8_t * null_sentinel() const noexcept {
            // We're using the address of column info to indicate
            // a NULL field.
            return reinterpret_cast<const tdsl::uint8_t *>(&column);
        }

        // --------------------------------------------------------------------------------

        // every command_context<T> is our friend.
        template <typename T>
        friend struct tdsl::detail::command_context;
    };
} // namespace tdsl

#endif