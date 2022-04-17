/**
 * _________________________________________________
 *
 * @file   tds_flag_types.hpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   17.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/detail/tds_inttypes.hpp>

namespace tdslite {
    // 0b1111'0011
    struct option_flags_1 {
        tdslite::uint8_t byte_order : 1; // 0 = ORDER_X86, 1 = ORDER_68000
        tdslite::uint8_t charset : 1;    // 0  = CHARSET_ASCII, 1 = CHARSET_EBCDIC
        tdslite::uint8_t float_type : 2; // 0 = FLOAT_IEEE_754, 1 = FLOAT_VAX, 2 = ND5000
        tdslite::uint8_t dump_load : 1;  // 0 = ON, 1 = OFF
        tdslite::uint8_t use_db : 1;     // 0 = USE_DB_OFF, 1 = USE_DB_ON
        tdslite::uint8_t database : 1;   // 0 = INIT_DB_WARN, 1 = INIT_DB_FATAL
        tdslite::uint8_t set_lang : 1;   // 0 = SET_LANG_OFF, 1 = SET_LANG_ON
    };

    struct option_flags_2 {
        tdslite::uint8_t language : 1; // 0 = INIT_LANG_WARN, 1 = INIT_LANG_FATAL
        tdslite::uint8_t odbc : 1;     // 0  = ODBC_OFF, 1 = ODBC_ON
        tdslite::uint8_t tran_boundary : 1;
        tdslite::uint8_t cache_connect : 1;
        tdslite::uint8_t user_type : 3;    // 0 = USER_NORMAL, 1 = USER_SERVER, 2 = USER_REMUSER, 3 = USER_SQLREPL
        tdslite::uint8_t int_security : 1; // 0 = INTEGRATED_SECURITY_OFF, 1 = INTEGRATED_SECURITY_ON
    };

    struct type_flags {
        tdslite::uint8_t sql_type : 4; // 0 = SQL_DFLT, 1 = SQL_TSQL
        tdslite::uint8_t ole_db : 1;   // 0 = OLEDB_OFF, 1 = OLEDB_ON
        tdslite::uint8_t read_only_intent : 1;
        tdslite::uint8_t reserved : 2;
    };

    struct option_flags_3 {
        tdslite::uint8_t change_password : 1; // 0 = No change request, 1 = Change request
        tdslite::uint8_t send_yukon_binary_xml : 1;
        tdslite::uint8_t user_instance : 1;
        tdslite::uint8_t unknown_collation_handling : 1;
        tdslite::uint8_t extension : 1;
        tdslite::uint8_t reserved : 3;
    };
} // namespace tdslite