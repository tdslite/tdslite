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
        /* Bit */
        BITTYPE       = 0x32,
        /* Smallint */
        INT2TYPE      = 0x34,
        /* Int */
        INT4TYPE      = 0x38,
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
}} // namespace tdsl::detail

#endif
