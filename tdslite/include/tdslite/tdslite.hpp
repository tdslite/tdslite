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

namespace tdsl {

    template <typename NetImpl>
    using driver    = detail::tdsl_driver<NetImpl>;

    using lang_code = detail::e_ms_lang_code_id;

    using data_type = detail::e_tds_data_type;

} // namespace tdsl

#endif
