/**
 * ____________________________________________________
 * Data types for TDS
 *
 * @file   tds_data_type.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSLITE_DETAIL_TDS_DATA_TYPE_HPP
#define TDSLITE_DETAIL_TDS_DATA_TYPE_HPP

#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl { namespace detail {

    /**
     *
     *
     */
    enum class e_tds_data_type : tdsl::uint8_t
    {
        /* Null */
        NULLTYPE      = 0x1,
        /* Tinyint */
        INT1TYPE      = 0x30,
        /* Alias for INT1TYPE */
        TINYINTTYPE   = INT1TYPE,
        /* Bit */
        BITTYPE       = 0x32,
        /* Smallint */
        INT2TYPE      = 0x34,
        /* Alias for INT2TYPE */
        SMALLINTTYPE  = INT2TYPE,
        /* Int */
        INT4TYPE      = 0x38,
        /* Alias for INT4TYPE */
        INTTYPE       = INT4TYPE,
        /* Smalldatetime */
        DATETIM4TYPE  = 0x3A,
        /* Real */
        FLT4TYPE      = 0x3B,
        /* Money */
        MONEYTYPE     = 0x3C,
        /* Datetime */
        DATETIMETYPE  = 0x3D,
        /* Float */
        FLT8TYPE      = 0x3E,
        /* Smallmoney */
        MONEY4TYPE    = 0x7A,
        /* Bigint */
        INT8TYPE      = 0x7F,
        /* Alias for INT8TYPE */
        BIGINTTYPE    = INT8TYPE,
        /* Unique identifier 16 byte binary */
        GUIDTYPE      = 0x24,
        /* N int */
        INTNTYPE      = 0x26,
        /* Decimal */
        DECIMALTYPE   = 0x37,
        /* Numeric */
        NUMERICTYPE   = 0x3F,
        /* N bit */
        BITNTYPE      = 0x68,
        /* Decimal N */
        DECIMALNTYPE  = 0x6A,
        /* Numeric with variable length */
        NUMERICNTYPE  = 0x6C,
        /* Float with variable length */
        FLTNTYPE      = 0x6D,
        /* Money with variable length*/
        MONEYNTYPE    = 0x6E,
        /* Date time with variable length*/
        DATETIMNTYPE  = 0x6F,
        /* Char (legacy support)*/
        CHARTYPE      = 0x2F,
        /*VarChar(legacy support)*/
        VARCHARTYPE   = 0x27,
        /*Binary(legacy support)*/
        BINARYTYPE    = 0x2D,
        VARBINARYTYPE = 0x25,
        /*VarBinary*/
        BIGVARBINTYPE = 0xA5,
        /* VarChar*/
        BIGVARCHRTYPE = 0xA7,
        /* Binary*/
        BIGBINARYTYPE = 0xAD,
        /*Char*/
        BIGCHARTYPE   = 0xAF,
        /*NVarChar*/
        NVARCHARTYPE  = 0xE7,
        /*NChar*/
        NCHARTYPE     = 0xEF,
        /* Text */
        TEXTTYPE      = 0x23,
        /* Image */
        IMAGETYPE     = 0x22,
        /* Ntext */
        NTEXTTYPE     = 0x63
    };

    enum class e_tds_data_size_type : tdsl::uint16_t
    {
        fixed,
        var_u8,
        var_u16,
        var_u32,
        var_precision,
        unknown
    };

    struct tds_data_type_properties {
        e_tds_data_size_type size_type;
        tdsl::uint16_t min_needed_bytes;
    };

    static inline tds_data_type_properties get_data_type_props(e_tds_data_type type) {
        tds_data_type_properties result{};
        switch (type) {
                // Fixed length data types
            case e_tds_data_type::NULLTYPE:
            case e_tds_data_type::BITTYPE:
            case e_tds_data_type::INT1TYPE:
            case e_tds_data_type::INT2TYPE:
            case e_tds_data_type::INT4TYPE:
            case e_tds_data_type::INT8TYPE:
            case e_tds_data_type::DATETIMETYPE:
            case e_tds_data_type::DATETIM4TYPE:
            case e_tds_data_type::FLT4TYPE:
            case e_tds_data_type::FLT8TYPE:
            case e_tds_data_type::MONEYTYPE:
            case e_tds_data_type::MONEY4TYPE:
            case e_tds_data_type::DECIMALTYPE: // legacy support
            case e_tds_data_type::NUMERICTYPE: // legacy support
                // No extra info to read for these
                // types. Their length is fixed.
                result.size_type = e_tds_data_size_type::fixed;
                break;
            case e_tds_data_type::DECIMALNTYPE:
            case e_tds_data_type::NUMERICNTYPE:
                // DECIMALNTYPE & NUMERICNTYPE have precision
                // and scale values. Precision determines the field's length
                // whereas scale is the multiplier.
                result.size_type        = e_tds_data_size_type::var_precision;
                result.min_needed_bytes = 2;
                break;
            // Variable length data types with 8-bit length bit width
            case e_tds_data_type::GUIDTYPE:
            case e_tds_data_type::INTNTYPE:
            case e_tds_data_type::BITNTYPE:
            case e_tds_data_type::FLTNTYPE:
            case e_tds_data_type::MONEYNTYPE:
            case e_tds_data_type::DATETIMNTYPE:
            case e_tds_data_type::CHARTYPE:
            case e_tds_data_type::VARCHARTYPE:
            case e_tds_data_type::BINARYTYPE:
            case e_tds_data_type::VARBINARYTYPE:
                result.size_type        = e_tds_data_size_type::var_u8;
                result.min_needed_bytes = sizeof(tdsl::uint8_t);
                break;
            // Variable length data types with 16-bit length bit width
            case e_tds_data_type::BIGVARBINTYPE:
            case e_tds_data_type::BIGCHARTYPE:
            case e_tds_data_type::NVARCHARTYPE:
            case e_tds_data_type::NCHARTYPE:
                result.size_type        = e_tds_data_size_type::var_u16;
                result.min_needed_bytes = sizeof(tdsl::uint16_t);
                break;
            // Variable length data types with 32-bit length bit width
            case e_tds_data_type::IMAGETYPE:
            case e_tds_data_type::NTEXTTYPE:
            case e_tds_data_type::TEXTTYPE:
                result.size_type        = e_tds_data_size_type::var_u32;
                result.min_needed_bytes = sizeof(tdsl::uint32_t);
                break;
            default:
                result.size_type = e_tds_data_size_type::unknown;
                break;
        }
        return result;
    }
}} // namespace tdsl::detail

#endif
