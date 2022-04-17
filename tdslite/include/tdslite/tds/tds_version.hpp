/**
 * _________________________________________________
 *
 * @file   tds_version.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   17.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/detail/tds_inttypes.hpp>

namespace tdslite {
    enum class e_tds_version : tdslite::uint32_t
    {
        sql_server_7_0      = 0x00000070,
        sql_server_2000     = 0x00000071,
        sql_server_2000_sp1 = 0x01000071,
        sql_server_2005     = 0x02000972,
        sql_server_2008     = 0x03000A73,
        sql_server_2008_r2  = 0x03000B73,
        sql_server_2012     = 0x04000074,
        sql_server_2014     = sql_server_2012,
        sql_server_2016     = sql_server_2012,
        sql_server_2017     = sql_server_2012,
        sql_server_2019     = sql_server_2012,
    };

} // namespace tdslite