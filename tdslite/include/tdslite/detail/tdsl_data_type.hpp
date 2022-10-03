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

#ifndef TDSL_DETAIL_TDS_DATA_TYPE_HPP
#define TDSL_DETAIL_TDS_DATA_TYPE_HPP

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

    /**
     * Data type properties
     */
    struct tds_data_type_properties {
        e_tds_data_size_type size_type;

        union {
            struct {
                tdsl::uint16_t length_size; // size of the length (only valid for variable length data types)
            } variable;

            tdsl::uint16_t fixed; // size of the data (only valid for fixed length data types)
        } length;

        struct {
            tdsl::uint8_t has_collation : 1;
            tdsl::uint8_t has_precision : 1;
            tdsl::uint8_t reserved : 6;
        } flags;

        /**
         * Returns true if data type is a variable size type
         */
        inline bool is_variable_size() const noexcept {
            switch (size_type) {
                case e_tds_data_size_type::var_precision:
                case e_tds_data_size_type::var_u8:
                case e_tds_data_size_type::var_u16:
                case e_tds_data_size_type::var_u32:
                    return true;
            }
            return false;
        }

        /**
         * Calculate minimum COLMETADATA bytes needed for
         * data type
         *
         * The function currently accounts for:
         * - Column name size (1 bytes)
         * - Collation size (5 bytes)
         * - Column data length size (N bytes, depending on type)
         *
         * @return tdsl::uint32_t Minimum COLMETADATA size for data type
         */
        inline auto min_colmetadata_size() const noexcept -> tdsl::uint32_t {
            constexpr int k_colname_size   = 1;
            constexpr int k_collation_size = 5;
            constexpr int k_precision_size = 2; // precision, scale

            tdsl::uint32_t final_size      = {0};
            final_size += (is_variable_size() ? length.variable.length_size : length.fixed);
            final_size += (flags.has_collation ? k_collation_size : 0);
            final_size += (flags.has_precision ? k_precision_size : 0);
            final_size += k_colname_size;
            return final_size;
        }
    };

    /**
     * Retrieve data type properties
     *
     * @param [in] type TDS data type
     *
     * @returns fixed_prop if @p type is a fixed data type
     * @returns varu8_prop if @p type is a variable data type in byte-size boundary
     * @returns varu16_prop if @p type is a variable data type in u16-size boundary
     * @returns varu32_prop if @p type is a variable data type in u32-size boundary
     * @returns varprec_prop if @p type is variable data type with precision and scale
     */
    static inline tds_data_type_properties get_data_type_props(e_tds_data_type type) {
        tds_data_type_properties result{};
        switch (type) {
            // Fixed length data types
            case e_tds_data_type::NULLTYPE:
                result.size_type    = e_tds_data_size_type::fixed;
                result.length.fixed = 0;
                break;
            case e_tds_data_type::BITTYPE:
            case e_tds_data_type::INT1TYPE:
                result.size_type    = e_tds_data_size_type::fixed;
                result.length.fixed = 1;
                break;
            case e_tds_data_type::INT2TYPE:
                result.size_type    = e_tds_data_size_type::fixed;
                result.length.fixed = 2;
                break;
            case e_tds_data_type::INT4TYPE:
            case e_tds_data_type::DATETIM4TYPE:
            case e_tds_data_type::FLT4TYPE:
            case e_tds_data_type::MONEY4TYPE:
                result.size_type    = e_tds_data_size_type::fixed;
                result.length.fixed = 4;
                break;
            case e_tds_data_type::INT8TYPE:
            case e_tds_data_type::DATETIMETYPE:
            case e_tds_data_type::FLT8TYPE:
            case e_tds_data_type::MONEYTYPE:
                result.size_type    = e_tds_data_size_type::fixed;
                result.length.fixed = 8;
                break;
            // MS-TDS document is not clear about length of these types
            // We can avoid them for now.
            // case e_tds_data_type::DECIMALTYPE: // legacy support
            // case e_tds_data_type::NUMERICTYPE: // legacy support
            //     // No extra info to read for these
            //     // types. Their length is fixed.
            //     result.size_type = e_tds_data_size_type::fixed;
            //     break;
            case e_tds_data_type::DECIMALNTYPE:
            case e_tds_data_type::NUMERICNTYPE:
                // DECIMALNTYPE & NUMERICNTYPE have precision
                // and scale values. Precision determines the field's length
                // whereas scale is the multiplier.
                result.size_type                   = e_tds_data_size_type::var_precision;
                result.length.variable.length_size = 2;
                result.flags.has_precision         = 1;
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
                result.size_type                   = e_tds_data_size_type::var_u8;
                result.length.variable.length_size = sizeof(tdsl::uint8_t);
                break;
            // Variable length data types with 16-bit length bit width
            case e_tds_data_type::BIGCHARTYPE:
            case e_tds_data_type::BIGVARCHRTYPE:
            case e_tds_data_type::NVARCHARTYPE:
            case e_tds_data_type::NCHARTYPE:
                result.flags.has_collation = true;
            // fallthrough
            case e_tds_data_type::BIGBINARYTYPE:
            case e_tds_data_type::BIGVARBINTYPE:
                // COLLATION occurs only if the type is BIGCHARTYPE, BIGVARCHARTYPE, TEXTTYPE, NTEXTTYPE,
                // NCHARTYPE, or NVARCHARTYPE.
                result.size_type                   = e_tds_data_size_type::var_u16;
                result.length.variable.length_size = sizeof(tdsl::uint16_t);
                break;
            // Variable length data types with 32-bit length bit width
            case e_tds_data_type::NTEXTTYPE:
            case e_tds_data_type::TEXTTYPE:
                result.flags.has_collation = true;
            // fallthrough
            case e_tds_data_type::IMAGETYPE:
                result.size_type                   = e_tds_data_size_type::var_u32;
                result.length.variable.length_size = sizeof(tdsl::uint32_t);
                break;
            default:
                result.size_type = e_tds_data_size_type::unknown;
                break;
        }
        return result;
    }
}} // namespace tdsl::detail

#endif
