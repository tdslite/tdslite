/**
 * _________________________________________________
 *
 * @file   tdsl_field.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSL_DETAIL_TDSL_FIELD_HPP
#define TDSL_DETAIL_TDSL_FIELD_HPP

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>

namespace tdsl {

    namespace sqltypes {
        using s_bit      = bool;
        using s_tinyint  = tdsl::uint8_t;
        using s_smallint = tdsl::int16_t;
        using s_int      = tdsl::int32_t;
        using s_bigint   = tdsl::int64_t;

        struct sql_data_type_base {};

        // using s_varchar  = tdsl::string_view;

        struct sql_money : public sql_data_type_base {
            explicit sql_money(tdsl::byte_view v) noexcept {
                // money is represented as an 8-byte signed integer. The TDS value is the money
                // value multiplied by 10^4. The 8-byte signed integer itself is represented in the
                // following sequence:
                // * One 4-byte integer that represents the more significant half.
                // * One 4-byte integer that represents the less significant half.
                tdsl::binary_reader<tdsl::endian::little> br{v};
                const tdsl::uint32_t msh = br.read<tdsl::uint32_t>();
                const tdsl::uint32_t lsh = br.read<tdsl::uint32_t>();
                value = static_cast<tdsl::int64_t>((static_cast<tdsl::uint64_t>(msh) << 32) |
                                                   (static_cast<tdsl::uint64_t>(lsh)));
            }

            inline tdsl::int64_t integer() const noexcept {
                return value / 10000;
            }

            inline tdsl::int64_t fraction() const noexcept {
                return value % 10000;
            }

            inline operator double() const noexcept {
                return static_cast<double>(integer()) + (static_cast<double>(fraction()) / 10000);
            }

            /**
             * Raw 8 byte integer stored in the database
             */
            inline tdsl::int64_t raw() const noexcept {
                return value;
            }

        private:
            tdsl::int64_t value;
        };

        // struct sql_decimal {
        //     inline operator tdsl::int64_t() const noexcept {
        //         return value;
        //     }

        //     tdsl::int64_t value;
        // };

        // using sql_numeric = sql_decimal;
    } // namespace sqltypes

    namespace detail {
        template <typename NetImpl>
        struct command_context;

        /**
         * Cast helper for integral types
         *
         * @param [in] data Data to cast
         * @return T bytes of data converted to host endianness and reinterpreted as type T
         */
        template <typename T, typename traits::enable_when::integral<T> = true>
        inline auto as_impl(byte_view data) -> T {
            TDSL_ASSERT_MSG(data.size_bytes() >= sizeof(T),
                            "Given span does not have enough bytes to read a value with type T!");
            return tdsl::binary_reader<tdsl::endian::little>{data}.read<T>();
        }

        template <typename T,
                  typename traits::enable_when::template_instance_of<T, tdsl::span> = true>
        inline auto as_impl(byte_view data) -> T {
            return data.rebind_cast<typename T::element_type>();
        }

        template <typename T,
                  typename traits::enable_when::base_of<sqltypes::sql_data_type_base, T> = true>
        inline auto as_impl(byte_view data) -> T {
            return T{data};
        }

        // Decimal is not yet supported.
        // /**
        //  * Cast helper for integral types
        //  *
        //  * @param [in] data Data to cast
        //  * @return T bytes of data converted to host endianness and reinterpreted as type T
        //  */
        // template <typename T, typename traits::enable_when::same<T, types::sql_decimal> = true>
        // inline auto as_impl(byte_view data) -> T {
        //     tdsl::binary_reader<tdsl::endian::little> reader{data};
        //     const bool sign = reader.read<bool>();
        //     // read N bytes
        //     // precision, scale
        //     // Can be 5, 9, 13, 17
        //     // TDSL_ASSERT_MSG(data.size_bytes() >= sizeof(T),
        //     //                 "Given span does not have enough bytes to read a value with type
        //     //                 T!");
        //     // return tdsl::binary_reader<tdsl::endian::little>{data}.read<T>();
        // }

    } // namespace detail

    /**
     * Non-owning field view.
     */
    struct tdsl_field : public byte_view {
        using byte_view::span;
        using byte_view::operator=;

        // (mgilor): Little hack to make implicit
        // construction on assignment work, e.g. tdsl_field = buf[50];
        template <typename... Args>
        inline tdsl_field & operator=(Args &&... args) {
            byte_view::operator=(TDSL_FORWARD(args)...);
            return *this;
        }

        template <typename T>
        inline auto as() const noexcept -> T {
            return detail::as_impl<T>(*this);
        }

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

    private:
        /**
         * Set this field as NULL.
         */
        void set_null() {
            (*this) = tdsl_field(null_sentinel(), null_sentinel());
            TDSL_ASSERT(data() == null_sentinel());
            TDSL_ASSERT(size() == 0);
        }

        /**
         * Data value that represents a NULL field.
         * This is the discrimination between an empty string
         * and a NULL string.
         */
        inline const tdsl::uint8_t * null_sentinel() const noexcept {
            /**
             * In order to save some space, we're using
             * a sentinel value instead of placing a NULL
             * flag member to tdsl_field.
             */
            static tdsl::uint8_t a{};
            return &a;
        }

        // every command_context<T> is our friend.
        template <typename T>
        friend struct tdsl::detail::command_context;
    };
} // namespace tdsl

#endif