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

    /**
     * Currently supported types are:
     * ------------------------------
     * TDS TYPE          SQL TYPE
     * ------------------------------
     * BITTYPE         - BIT
     - BITNTYPE        - (BIT)
     * INT1TYPE        - TINYINT
     * INT2TYPE        - SMALLINT
     * INT4TYPE        - INT
     * INT8TYPE        - BIGINT
     - INTNTYPE        - (TINYINT, SMALLINT, INT or BIGINT, depending on size)
     * FLT4TYPE        - REAL
     * FLT8TYPE        - FLOAT
     * FLTNTYPE        - (REAL or FLOAT, depending on size)
     * NVARCHARTYPE    - NVARCHAR(N)
     * NCHARTYPE       - NCHAR(N)
     * BIGVARCHARTYPE  - VARCHAR(N)
     * BIGCHARTYPE     - CHAR(N)
     * GUIDTYPE        - UNIQUEIDENTIFIER
     * BIGBINARYTYPE   - BINARY(N)
     * BIGVARBINTYPE   - VARBINARY(N)
     * ------------------------------
     * NOTE: TEXTTYPE(TEXT), NTEXTTYPE(NTEXT) and IMAGETYPE(IMAGE)
     * are deprecated in favor of VARCHAR(MAX), NVARCHAR(MAX)
     * and VARBINARY(MAX) respectively, thus, will not be
     * supported.
     */

    struct sql_parameter_binding {
        e_tds_data_type type;
        tdsl::byte_view value;
        tdsl::uint32_t type_size{}; // required for some types only
    };

    // --------------------------------------------------------------------------------

    // Tag type to enable sql parameter implementation for arithmetic types
    struct arithmetic_tag {};

    // --------------------------------------------------------------------------------

    // Tag type to enable sql parameter implementation for byte-view like types
    struct byte_view_tag {};

    // --------------------------------------------------------------------------------

    template <e_tds_data_type, typename Enabler = void>
    struct sql_param_traits;

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::BITTYPE> {
        using type = bool;
        using tag  = arithmetic_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::INT1TYPE> {
        using type = tdsl::uint8_t;
        using tag  = arithmetic_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::INT2TYPE> {
        using type = tdsl::int16_t;
        using tag  = arithmetic_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::INT4TYPE> {
        using type = tdsl::int32_t;
        using tag  = arithmetic_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::INT8TYPE> {
        using type = tdsl::int64_t;
        using tag  = arithmetic_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::FLT4TYPE> {
        using type = float;
        using tag  = arithmetic_tag;
        static_assert(sizeof(float) == 4,
                      "The implementation assumes that float is 4 bytes in size!");
    };

    // // --------------------------------------------------------------------------------
    // FIXME: Make this conditional

    // template <>
    // struct sql_param_traits<e_tds_data_type::FLT8TYPE> {
    //     using type = double;
    //     using tag  = arithmetic_tag;

    //     inline sql_param_traits() {
    //         static_assert(traits::dependent_bool<sizeof(double) == 8>::value,
    //                       "This type cannot be used since sizeof(double) != 8 in your
    //                       platform!");
    //     }
    // };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::NVARCHARTYPE> {
        using type = tdsl::wstring_view;
        using tag  = byte_view_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::NCHARTYPE> {
        using type = tdsl::wstring_view;
        using tag  = byte_view_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::BIGVARCHRTYPE> {
        using type = tdsl::string_view;
        using tag  = byte_view_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::BIGCHARTYPE> {
        using type = tdsl::string_view;
        using tag  = byte_view_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::GUIDTYPE> {
        using type = tdsl::byte_view;
        using tag  = byte_view_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::BIGBINARYTYPE> {
        using type = tdsl::byte_view;
        using tag  = byte_view_tag;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::BIGVARBINTYPE> {
        using type = tdsl::byte_view;
        using tag  = byte_view_tag;
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
    struct sql_parameter_impl<DTYPE, arithmetic_tag> {
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
            sql_parameter_binding param = {};
            param.type                  = DTYPE;
            param.type_size             = sizeof(BackingType);
            param.value                 = to_bytes();
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
    struct sql_parameter_impl<DTYPE, byte_view_tag> {
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
            sql_parameter_binding param = {};
            param.type                  = DTYPE;
            param.type_size             = value.size_bytes();
            param.value                 = to_bytes();
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
    // TDSL_DATA_TYPE_DECL(BITTYPE       , 0x32) TDSL_DATA_TYPE_LIST_DELIM BIT
    // TDSL_DATA_TYPE_DECL(INT1TYPE      , 0x30) TDSL_DATA_TYPE_LIST_DELIM TINYINT
    // TDSL_DATA_TYPE_DECL(INT2TYPE      , 0x34) TDSL_DATA_TYPE_LIST_DELIM SMALLINT
    // TDSL_DATA_TYPE_DECL(INT4TYPE      , 0x38) TDSL_DATA_TYPE_LIST_DELIM INT
    // TDSL_DATA_TYPE_DECL(INT8TYPE      , 0x7F) TDSL_DATA_TYPE_LIST_DELIM BIGINT
    // TDSL_DATA_TYPE_DECL(FLT4TYPE      , 0x3B) TDSL_DATA_TYPE_LIST_DELIM REAL
    // TDSL_DATA_TYPE_DECL(FLT8TYPE      , 0x3E) TDSL_DATA_TYPE_LIST_DELIM FLOAT
    // TDSL_DATA_TYPE_DECL(BIGVARCHRTYPE , 0xA7) TDSL_DATA_TYPE_LIST_DELIM VARCHAR(N)
    // TDSL_DATA_TYPE_DECL(NVARCHARTYPE  , 0xE7) TDSL_DATA_TYPE_LIST_DELIM NVARCHAR(N)
    // TDSL_DATA_TYPE_DECL(GUIDTYPE      , 0x24) TDSL_DATA_TYPE_LIST_DELIM UNIQUEIDENTIFIER
    // TDSL_DATA_TYPE_DECL(NCHARTYPE     , 0xEF) TDSL_DATA_TYPE_LIST_DELIM NCHAR(N)
    // TDSL_DATA_TYPE_DECL(BIGCHARTYPE   , 0xAF) TDSL_DATA_TYPE_LIST_DELIM CHAR(N)
    // TDSL_DATA_TYPE_DECL(BIGVARBINTYPE , 0xA5) TDSL_DATA_TYPE_LIST_DELIM VARBINARY(N)
    // TDSL_DATA_TYPE_DECL(BIGBINARYTYPE , 0xAD) TDSL_DATA_TYPE_LIST_DELIM BINARY(N)
    // TDSL_DATA_TYPE_DECL(INTNTYPE      , 0x26) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(BITNTYPE      , 0x68) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(FLTNTYPE      , 0x6D) TDSL_DATA_TYPE_LIST_DELIM

    // Not yet implemented:
    // TDSL_DATA_TYPE_DECL(DATETIM4TYPE  , 0x3A) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(DATETIMETYPE  , 0x3D) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(DATETIMNTYPE  , 0x6F) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(MONEYTYPE     , 0x3C) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(MONEY4TYPE    , 0x7A) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(MONEYNTYPE    , 0x6E) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(DECIMALTYPE   , 0x37) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(DECIMALNTYPE  , 0x6A) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(NUMERICTYPE   , 0x3F) TDSL_DATA_TYPE_LIST_DELIM
    // TDSL_DATA_TYPE_DECL(NUMERICNTYPE  , 0x6C) TDSL_DATA_TYPE_LIST_DELIM

    using sql_parameter_bit       = sql_parameter<e_tds_data_type::BITTYPE>;
    using sql_parameter_tinyint   = sql_parameter<e_tds_data_type::INT1TYPE>;
    using sql_parameter_smallint  = sql_parameter<e_tds_data_type::INT2TYPE>;
    using sql_parameter_int       = sql_parameter<e_tds_data_type::INT4TYPE>;
    using sql_parameter_bigint    = sql_parameter<e_tds_data_type::INT8TYPE>;
    using sql_parameter_float4    = sql_parameter<e_tds_data_type::FLT4TYPE>;
    // using sql_parameter_float8    = sql_parameter<e_tds_data_type::FLT8TYPE>;
    using sql_parameter_varchar   = sql_parameter<e_tds_data_type::BIGVARCHRTYPE>;
    using sql_parameter_char      = sql_parameter<e_tds_data_type::BIGCHARTYPE>;
    using sql_parameter_nvarchar  = sql_parameter<e_tds_data_type::NVARCHARTYPE>;
    using sql_parameter_nchar     = sql_parameter<e_tds_data_type::NCHARTYPE>;
    using sql_parameter_guid      = sql_parameter<e_tds_data_type::GUIDTYPE>;
    using sql_parameter_binary    = sql_parameter<e_tds_data_type::BIGBINARYTYPE>;
    using sql_parameter_varbinary = sql_parameter<e_tds_data_type::BIGVARBINTYPE>;

    // float & real

}} // namespace tdsl::detail

#endif