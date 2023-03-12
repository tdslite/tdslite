/**
 * ____________________________________________________
 *
 * @file   tdsl_tds_procedure_id.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   23.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TDS_PROCEDURE_ID_HPP
#define TDSL_DETAIL_TDS_PROCEDURE_ID_HPP

#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl { namespace detail {

    // --------------------------------------------------------------------------------

    enum class e_proc_id : tdsl::uint8_t
    {
        sp_cursor         = 1,
        sp_cursoropen     = 2,
        sp_cursorprepare  = 3,
        sp_cursorexecute  = 4,
        sp_cursorprepexec = 5,
        sp_cursorfetch    = 7,
        sp_cursoroption   = 8,
        sp_cursorclose    = 9,
        sp_executesql     = 10,
        sp_prepare        = 11,
        sp_execute        = 12,
        sp_prepexec       = 13,
        sp_prepexecrpc    = 14,
        sp_unprepare      = 15
    };

    // --------------------------------------------------------------------------------

    enum class e_rpc_mode : tdsl::uint8_t
    {
        executesql = static_cast<tdsl::uint8_t>(e_proc_id::sp_executesql),
        prepexec   = static_cast<tdsl::uint8_t>(e_proc_id::sp_prepexec),
    };

    // --------------------------------------------------------------------------------

    enum class e_rpc_error_code : tdsl::uint8_t
    {
        invalid_mode = 1,
    };
}} // namespace tdsl::detail

#endif
