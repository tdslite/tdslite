/**
 * _________________________________________________
 *
 * @file   tdsl_mssql_error_codes.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   23.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSL_DETAIL_MSSQL_ERROR_CODE_HPP
#define TDSL_DETAIL_MSSQL_ERROR_CODE_HPP

#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl {
    enum class e_mssql_error_code : tdsl::int32_t
    {
        logon_failed = 18456
    };
} // namespace tdsl

#endif