/**
 * _________________________________________________
 *
 * @file   tdsl_driver.hpp
 * @author Mustafa Kemal GILOR <mgilor@nettsi.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

namespace tdsl { namespace detail {

    /**
     * tds-lite driver
     *
     * @tparam NetImpl The networking implementation type
     */
    template <typename NetImpl>
    struct tdsl_driver {};

}} // namespace tdsl::detail