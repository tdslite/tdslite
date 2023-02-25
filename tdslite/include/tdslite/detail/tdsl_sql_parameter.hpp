/**
 * ____________________________________________________
 * SQL RPC parameter types
 *
 * @file   tdsl_sql_parameter.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   10.01.2023
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TDSL_SQL_PARAMETER_HPP
#define TDSL_DETAIL_TDSL_SQL_PARAMETER_HPP

#include <tdslite/detail/tdsl_data_type.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_byte_swap.hpp>
#include <tdslite/util/tdsl_string_view.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>

namespace tdsl { namespace detail {

    struct sql_parameter_binding {
        e_tds_data_type type;
        tdsl::byte_view value;
        tdsl::uint32_t type_size{}; // required for some types only
    };

    // --------------------------------------------------------------------------------

    // Tag type to enable sql parameter implementation for integral types
    struct integral_tag {};

    // --------------------------------------------------------------------------------

    // Tag type to enable sql parameter implementation for string types
    struct string_view_tag {};

    // --------------------------------------------------------------------------------

    // Tag type to enable sql parameter implementation for float types
    using float_tag = integral_tag;

    // --------------------------------------------------------------------------------

    template <e_tds_data_type>
    struct sql_param_traits;

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::BITTYPE> {
        using type = bool;
        using tag  = integral_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::INT1TYPE> {
        using type = tdsl::uint8_t;
        using tag  = integral_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::INT2TYPE> {
        using type = tdsl::int16_t;
        using tag  = integral_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::INT4TYPE> {
        using type = tdsl::int32_t;
        using tag  = integral_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::INT8TYPE> {
        using type = tdsl::int64_t;
        using tag  = integral_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::FLT4TYPE> {
        using type = float;
        using tag  = float_tag;
        static_assert(sizeof(float) == 4,
                      "The implementation assumes that float is 4 bytes in size!");
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::FLT8TYPE> {
        using type = double;
        using tag  = float_tag;
        static_assert(sizeof(double) == 8,
                      "The implementation assumes that float is 4 bytes in size!");
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::NVARCHARTYPE> {
        using type = tdsl::wstring_view;
        using tag  = string_view_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::BIGVARCHRTYPE> {
        using type = tdsl::string_view;
        using tag  = string_view_tag;
    };

    // --------------------------------------------------------------------------------

    template <e_tds_data_type DTYPE, typename T>
    struct sql_parameter_impl;

    // --------------------------------------------------------------------------------

    /**
     * SQL parameter implementation
     * (integral)
     *
     * @tparam DTYPE Integral type
     */
    template <e_tds_data_type DTYPE>
    struct sql_parameter_impl<DTYPE, integral_tag> {
        using BackingType = typename sql_param_traits<DTYPE>::type;

        // --------------------------------------------------------------------------------

        inline sql_parameter_impl() : value{} {}

        // --------------------------------------------------------------------------------

        inline sql_parameter_impl(BackingType value) : value(native_to_le(value)) {}

        // --------------------------------------------------------------------------------

        /**
         * Act as backing type for const operations.
         *
         * @return backing_type backing type value in native
         *         byte order
         */
        inline operator BackingType() const noexcept {
            return le_to_native(value);
        }

        // --------------------------------------------------------------------------------

        /**
         * Cast operator to sql_paramater_binding
         */
        inline TDSL_NODISCARD operator sql_parameter_binding() const noexcept {
            sql_parameter_binding param{};
            param.type      = DTYPE;
            param.type_size = sizeof(BackingType);
            param.value     = to_bytes();
            return param;
        }

    private:
        /**
         * Get @ref value as bytes
         * (integral)
         */
        inline TDSL_NODISCARD auto to_bytes() const noexcept -> tdsl::byte_view {
            return tdsl::byte_view{reinterpret_cast<const tdsl::uint8_t *>(&value),
                                   sizeof(BackingType)};
        }

        // backing type for parameter, stored as
        // little-endian byte order regardless of
        // native byte order.
        BackingType value;
    };

    // --------------------------------------------------------------------------------

    template <e_tds_data_type DTYPE>
    struct sql_parameter_impl<DTYPE, string_view_tag> {
        using BackingType = typename sql_param_traits<DTYPE>::type;

        // --------------------------------------------------------------------------------

        inline sql_parameter_impl() : value{} {}

        // --------------------------------------------------------------------------------

        inline sql_parameter_impl(BackingType value) : value(value) {}

        // --------------------------------------------------------------------------------

        /**
         * Act as backing type for const operations.
         *
         * @return backing_type backing type value in native
         *         byte order
         */
        inline operator BackingType() const noexcept {
            return value;
        }

        // --------------------------------------------------------------------------------

        /**
         * Cast operator to sql_paramater_binding
         */
        inline TDSL_NODISCARD operator sql_parameter_binding() const noexcept {
            sql_parameter_binding param{};
            param.type      = DTYPE;
            param.type_size = value.size_bytes(); // FIXME: Probably wrong
            param.value     = to_bytes();
            return param;
        }

    private:
        /**
         * Get @ref value as bytes
         */
        inline TDSL_NODISCARD auto to_bytes() const noexcept -> tdsl::byte_view {
            return value.template rebind_cast<const tdsl::uint8_t>();
        }

        // backing type for parameter, stored as
        // little-endian byte order regardless of
        // native byte order.
        BackingType value;
    };

    // --------------------------------------------------------------------------------

    template <e_tds_data_type DTYPE,
              typename Impl = sql_parameter_impl<DTYPE, typename sql_param_traits<DTYPE>::tag>>
    struct sql_parameter : public Impl {
        // Inherit constructors
        using Impl::Impl;
    };

    // --------------------------------------------------------------------------------

    // Implemented:
    // TDSL_DATA_TYPE_DECL(BITTYPE       , 0x32) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(INT1TYPE      , 0x30) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(INT2TYPE      , 0x34) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(INT4TYPE      , 0x38) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(INT8TYPE      , 0x7F) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(FLT4TYPE      , 0x3B) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(FLT8TYPE      , 0x3E) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(BIGVARCHRTYPE , 0xA7) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(NVARCHARTYPE  , 0xE7) TDSL_DATA_TYPE_LIST_DELIM

    // Not yet implemented:
    // TDSL_DATA_TYPE_DECL(GUIDTYPE      , 0x24) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(DATETIM4TYPE  , 0x3A) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(DATETIMETYPE  , 0x3D) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(MONEYTYPE     , 0x3C) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(MONEY4TYPE    , 0x7A) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(INTNTYPE      , 0x26) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(DECIMALTYPE   , 0x37) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(NUMERICTYPE   , 0x3F) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(BITNTYPE      , 0x68) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(DECIMALNTYPE  , 0x6A) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(NUMERICNTYPE  , 0x6C) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(FLTNTYPE      , 0x6D) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(DATETIMNTYPE  , 0x6F) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(MONEYNTYPE    , 0x6E) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(BIGVARBINTYPE , 0xA5) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(BIGBINARYTYPE , 0xAD) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(BIGCHARTYPE   , 0xAF) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(NCHARTYPE     , 0xEF) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(TEXTTYPE      , 0x23) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(IMAGETYPE     , 0x22) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(NTEXTTYPE     , 0x63)

    using sql_parameter_bit      = sql_parameter<e_tds_data_type::BITTYPE>;
    using sql_parameter_tinyint  = sql_parameter<e_tds_data_type::INT1TYPE>;
    using sql_parameter_smallint = sql_parameter<e_tds_data_type::INT2TYPE>;
    using sql_parameter_int      = sql_parameter<e_tds_data_type::INT4TYPE>;
    using sql_parameter_bigint   = sql_parameter<e_tds_data_type::INT8TYPE>;
    using sql_parameter_float4   = sql_parameter<e_tds_data_type::FLT4TYPE>;
    using sql_parameter_float8   = sql_parameter<e_tds_data_type::FLT8TYPE>;
    using sql_parameter_varchar  = sql_parameter<e_tds_data_type::BIGVARCHRTYPE>;
    using sql_parameter_nvarchar = sql_parameter<e_tds_data_type::NVARCHARTYPE>;

    // float & real

}} // namespace tdsl::detail

#endif