/**
 * ____________________________________________________
 * FIXME: Description
 *
 * @file   tdsl_field.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
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
        using s_float4   = float;
        using s_float8   = double;

        struct sql_data_type_base {};

        /**
         * money sql type
         */
        struct sql_money : public sql_data_type_base {

            /**
             * Construct a new sql money object
             *
             * @param [in] v View to bytes to be interpreted as sql_money
             */
            inline explicit sql_money(tdsl::byte_view v) noexcept {
                TDSL_ASSERT(v.size_bytes() == (sizeof(tdsl::uint32_t) * 2));
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

            // --------------------------------------------------------------------------------

            /**
             * Calculate the integer part
             *
             * @return tdsl::int64_t sql_money integer part
             */
            inline TDSL_NODISCARD tdsl::int64_t integer() const noexcept {
                return value / 10000;
            }

            // --------------------------------------------------------------------------------

            /**
             * Calculate the fraction part
             *
             * @return tdsl::int64_t sql_money fraction part
             */
            inline TDSL_NODISCARD tdsl::int64_t fraction() const noexcept {
                return value % 10000;
            }

            // --------------------------------------------------------------------------------

            /**
             * Get sql_money value as a double
             *
             * @return double sql_money as double
             */
            inline operator double() const noexcept {
                return static_cast<double>(integer()) + (static_cast<double>(fraction()) / 10000);
            }

            // --------------------------------------------------------------------------------

            /**
             * Raw 8 byte integer stored in the database
             */
            inline TDSL_NODISCARD tdsl::int64_t raw() const noexcept {
                return value;
            }

        private:
            tdsl::int64_t value;
        };

        /**
         * smalldatetime sql type
         */
        struct sql_smalldatetime : public sql_data_type_base {

            // --------------------------------------------------------------------------------

            /**
             * Construct a new SQL smalldatetime object
             *
             * @param [in] v View to bytes to be interpreted as sql_smalldatetime
             */
            inline explicit sql_smalldatetime(tdsl::byte_view v) noexcept {
                TDSL_ASSERT(v.size_bytes() == (sizeof(tdsl::uint16_t) * 2));
                tdsl::binary_reader<tdsl::endian::little> br{v};
                days_elapsed    = br.read<tdsl::uint16_t>();
                minutes_elapsed = br.read<tdsl::uint16_t>();
            }

            // --------------------------------------------------------------------------------

            /**
             * Convert smalldatetime to unix timestamp
             *
             * @return tdsl::uint64_t smalldatetime value as unix timestamp
             */
            inline tdsl::uint64_t to_unix_timestamp() const noexcept {
                constexpr auto days_between_epochs = ((1970 - 1900) * 365);
                if (days_elapsed < days_between_epochs) {
                    return 0; // 1-1-1970
                }
                return ((days_elapsed - days_between_epochs) * 86400) + (minutes_elapsed * 60);
            }

            // --------------------------------------------------------------------------------

            // One 2-byte unsigned integer that represents the
            // number of days since January 1, 1900.
            tdsl::uint16_t days_elapsed;
            // One 2-byte unsigned integer that represents the
            // number of minutes elapsed since 12 AM that day.
            tdsl::uint16_t minutes_elapsed;
        };

        /**
         * datetime sql type
         */
        struct sql_datetime : public sql_data_type_base {

            // --------------------------------------------------------------------------------

            /**
             * Construct a new SQL smalldatetime object
             *
             * @param [in] v View to bytes to be interpreted as sql_smalldatetime
             */
            inline explicit sql_datetime(tdsl::byte_view v) noexcept {
                TDSL_ASSERT(v.size_bytes() == (sizeof(tdsl::int32_t) + sizeof(uint32_t)));
                tdsl::binary_reader<tdsl::endian::little> br{v};
                days_elapsed         = br.read<tdsl::int32_t>();
                centiseconds_elapsed = br.read<tdsl::uint32_t>();
            }

            // --------------------------------------------------------------------------------

            /**
             * Convert datetime to unix timestamp
             *
             * @return tdsl::uint64_t datetime value as unix timestamp
             */
            inline tdsl::uint64_t to_unix_timestamp() const noexcept {
                constexpr auto days_between_epochs = ((1970 - 1900) * 365);
                if (days_elapsed < days_between_epochs) {
                    return 0; // 1-1-1970
                }
                return ((days_elapsed - days_between_epochs) * 86400ul) +
                       (centiseconds_elapsed / 100);
            }

            // --------------------------------------------------------------------------------

            // One 4-byte signed integer that represents the number of days
            // since January 1, 1900. Negative numbers are allowed to represent
            // dates since January 1, 1753.
            tdsl::int32_t days_elapsed;
            // One 4-byte unsigned integer that represents the number of one
            // three-hundredths of a second (300 counts per second) elapsed
            // since 12 AM that day.
            tdsl::uint32_t centiseconds_elapsed;
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

        // --------------------------------------------------------------------------------

        /**
         * Cast helper for arithmetic types
         *
         * @param [in] data Data to cast
         * @return T bytes of data converted to host endianness and reinterpreted as type T
         */
        template <typename T, typename traits::enable_when::arithmetic<T> = true>
        inline auto as_impl(byte_view data) -> T {
            TDSL_ASSERT_MSG(data.size_bytes() >= sizeof(T),
                            "Given span does not have enough bytes to read a value with type T!");
            return tdsl::binary_reader<tdsl::endian::little>{data}.read<T>();
        }

        // --------------------------------------------------------------------------------

        template <typename T,
                  typename traits::enable_when::template_instance_of<T, tdsl::span> = true>
        inline auto as_impl(byte_view data) -> T {
            return data.rebind_cast<typename T::element_type>();
        }

        // --------------------------------------------------------------------------------

        template <typename T,
                  typename traits::enable_when::base_of<sqltypes::sql_data_type_base, T> = true>
        inline auto as_impl(byte_view data) -> T {
            return T{data};
        }

        // --------------------------------------------------------------------------------

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
     * Non-owning view of a row field.
     */
    struct tdsl_field : public byte_view {
        using byte_view::span;
        using byte_view::operator=;

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
            return detail::as_impl<T>(*this);
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

    private:
        /**
         * Set this field as NULL.
         */
        void set_null() {
            (*this) = tdsl_field(null_sentinel(), null_sentinel());
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
            /**
             * In order to save some space, we're using
             * a sentinel value instead of placing a NULL
             * flag member to tdsl_field.
             */
            static tdsl::uint8_t a{};
            return &a;
        }

        // --------------------------------------------------------------------------------

        // every command_context<T> is our friend.
        template <typename T>
        friend struct tdsl::detail::command_context;
    };
} // namespace tdsl

#endif