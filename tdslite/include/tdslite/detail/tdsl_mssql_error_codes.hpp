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

#pragma once

#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl {
    enum class e_mssql_error_code : tdsl::int32_t
    { logon_failed = 18456 };
} // namespace tdsl