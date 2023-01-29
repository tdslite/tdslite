/**
 * ____________________________________________________
 * Header-only, lightweight implementation of TDS
 * (tabular data stream) protocol to interact with
 * database engines such as (MSSQL)
 *
 * @file   tdslite.hpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_TDSL_MAIN_HPP
#define TDSL_TDSL_MAIN_HPP

#include <tdslite/detail/tdsl_driver.hpp>
#include <tdslite/detail/tdsl_lang_code_id.hpp>
#include <tdslite/detail/tdsl_tds_procedure_id.hpp>

namespace tdsl {

    template <typename NetImpl>
    using driver    = detail::tdsl_driver<NetImpl>;

    using lang_code = detail::e_ms_lang_code_id;

    using data_type = detail::e_tds_data_type;

    using detail::sql_parameter;
    using detail::sql_parameter_bigint;
    using detail::sql_parameter_binding;
    using detail::sql_parameter_int;
    using detail::sql_parameter_nvarchar;
    using detail::sql_parameter_smallint;
    using detail::sql_parameter_tinyint;
    using detail::sql_parameter_varchar;

    using rpc_mode    = detail::e_rpc_mode;
    using stored_proc = detail::e_proc_id;

} // namespace tdsl

#endif
