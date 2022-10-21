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
#include <tdslite/util/tdsl_macrodef.hpp>

namespace tdsl { namespace detail {

    enum class e_tds_data_type : tdsl::uint8_t
    {
    // clang-format off
        #define TDSL_DATA_TYPE_DECL(NAME, VALUE) NAME = VALUE
        #define TDSL_DATA_TYPE_LIST_DELIM        ,
        #include <tdslite/detail/tdsl_data_type.inc>
        // clang-format on
    };

    /**
     * Translate data type value @type to string
     *
     * @param [in] type Token type
     * @return Token type as string if @p type has a corresponding string representation,
     * "UNDEFINED" otherwise.
     */
    inline static TDSL_CXX14_CONSTEXPR const char *
    data_type_to_str(e_tds_data_type type) noexcept {

        switch (type) {
// clang-format off
            #define TDSL_DATA_TYPE_DECL(NAME, VALUE)                                                           \
                case e_tds_data_type::NAME:                                                                    \
                    return #NAME "(" #VALUE ")";
            #define TDSL_DATA_TYPE_LIST_DELIM ;
            #include <tdslite/detail/tdsl_data_type.inc>
                // clang-format on
            }

            return "UNDEFINED";
        }

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
                    tdsl::uint16_t length_size; // size of the length (only valid for variable
                                                // length data types)
                } variable;

                tdsl::uint16_t fixed; // size of the data (only valid for fixed length data types)
            } length;

            struct {
                tdsl::uint8_t has_collation : 1;
                tdsl::uint8_t has_precision : 1;
                tdsl::uint8_t has_table_name : 1;
                tdsl::uint8_t has_textptr : 1;
                // Char and binary data types have values that either are null or are 0 to 65534
                // (0x0000 to 0xFFFE) bytes of data. Null is represented by a length of 65535
                // (0xFFFF). A non-nullable char or binary can still have a length of zero (for
                // example, an empty value). A program that MUST pad a value to a fixed length
                // typically adds blanks to the end of a char and adds binary zeros to the end of a
                // binary. Text and image data types have values that either are null or are 0 to 2
                // gigabytes (0x00000000 to 0x7FFFFFFF bytes) of data. Null is represented by a
                // length of -1 (0xFFFFFFFF). No other length specification is supported
                tdsl::uint8_t maxlen_represents_null : 1;
                // .. Other nullable data types have a length of 0 when they are null.
                tdsl::uint8_t zero_represents_null : 1;
                tdsl::uint8_t reserved : 2;
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
                    default:
                        return false;
                }
                TDSL_UNREACHABLE;
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
                constexpr int k_tablename_size = 2; // UCS-2 string, so 2 bytes len.
                constexpr int k_collation_size = 5;
                constexpr int k_precision_size = 2; // precision, scale

                tdsl::uint32_t final_size      = {0};
                final_size += (is_variable_size() ? length.variable.length_size : length.fixed);
                final_size += (flags.has_collation ? k_collation_size : 0);
                final_size += (flags.has_precision ? k_precision_size : 0);
                final_size += (flags.has_table_name ? k_tablename_size : 0);
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
                case e_tds_data_type::DECIMALNTYPE:
                case e_tds_data_type::NUMERICNTYPE:
                    // DECIMALNTYPE & NUMERICNTYPE have precision
                    // and scale values. Precision determines the field's length
                    // whereas scale is the multiplier.
                    result.size_type                   = e_tds_data_size_type::var_precision;
                    result.length.variable.length_size = 2;
                    result.flags.has_precision         = {true};
                    break;
                // Variable length data types with 8-bit length bit width
                case e_tds_data_type::GUIDTYPE:
                case e_tds_data_type::INTNTYPE:
                case e_tds_data_type::BITNTYPE:
                case e_tds_data_type::FLTNTYPE:
                case e_tds_data_type::MONEYNTYPE:
                case e_tds_data_type::DATETIMNTYPE:
                    // Nullable values are returned by using the INTNTYPE, BITNTYPE, FLTNTYPE,
                    // GUIDTYPE, MONEYNTYPE, and DATETIMNTYPE tokens which will use the length byte
                    // to specify the length of the value or GEN_NULL as appropriate.
                    result.flags.zero_represents_null  = {true};
                    result.size_type                   = e_tds_data_size_type::var_u8;
                    result.length.variable.length_size = sizeof(tdsl::uint8_t);
                    break;
                // Variable length data types with 16-bit length bit width
                case e_tds_data_type::BIGCHARTYPE:
                case e_tds_data_type::BIGVARCHRTYPE:
                case e_tds_data_type::NVARCHARTYPE:
                case e_tds_data_type::NCHARTYPE:
                    result.flags.has_collation = {true};
                // fallthrough
                case e_tds_data_type::BIGBINARYTYPE:
                case e_tds_data_type::BIGVARBINTYPE:
                    // COLLATION occurs only if the type is BIGCHARTYPE, BIGVARCHARTYPE, TEXTTYPE,
                    // NTEXTTYPE, NCHARTYPE, or NVARCHARTYPE.
                    result.flags.maxlen_represents_null = {true};
                    result.size_type                    = e_tds_data_size_type::var_u16;
                    result.length.variable.length_size  = sizeof(tdsl::uint16_t);
                    break;
                // Variable length data types with 32-bit length bit width
                case e_tds_data_type::NTEXTTYPE:
                case e_tds_data_type::TEXTTYPE:
                    result.flags.has_collation = {true};
                // fallthrough
                case e_tds_data_type::IMAGETYPE:
                    // The TableName element is specified only if a text, ntext,
                    // or image column is included in the result
                    result.flags.has_textptr            = {true};
                    result.flags.has_table_name         = {true};
                    result.flags.maxlen_represents_null = {true};
                    result.size_type                    = e_tds_data_size_type::var_u32;
                    result.length.variable.length_size  = sizeof(tdsl::uint32_t);
                    break;
                default:
                    result.size_type = e_tds_data_size_type::unknown;
                    break;
            }
            return result;
        }

        static inline bool is_valid_variable_length_for_type(e_tds_data_type type,
                                                             tdsl::uint32_t length) noexcept {

            if (length == 0x00) {
                // For all variable length data types, the value is 0x00 for NULL instances.
                return true;
            }

            switch (type) {
                case e_tds_data_type::DECIMALNTYPE:
                case e_tds_data_type::NUMERICNTYPE: {
                    switch (length) {
                        case 0x05:
                        case 0x09:
                        case 0x0d:
                        case 0x11:
                            return true;
                    }
                    return false;
                }
                case e_tds_data_type::MONEYNTYPE:
                case e_tds_data_type::DATETIMNTYPE:
                case e_tds_data_type::FLTNTYPE:
                    return length == 0x04 || length == 0x08;
                case e_tds_data_type::INTNTYPE:
                    switch (length) {
                        case 0x01:
                        case 0x02:
                        case 0x04:
                        case 0x08:
                            return true;
                    }
                    break;

                case e_tds_data_type::GUIDTYPE:
                    return length == 0x10;
                case e_tds_data_type::BITNTYPE:
                    return length == 0x01;

                default:
                    return true;
            }

            TDSL_UNREACHABLE;
        }
}} // namespace tdsl::detail

#endif
