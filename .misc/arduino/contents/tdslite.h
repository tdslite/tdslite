/**
 * ____________________________________________________
 * Header-only, lightweight implementation of TDS
 * (tabular data stream) protocol to interact with
 * database engines such as (MSSQL)
 *
 * @file   tdslite.h
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   11.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_TDSL_MAIN_ARDUINO_HPP
#define TDSL_TDSL_MAIN_ARDUINO_HPP

// Arduino library format expects a top-level
// header file to be present, so this file acts
// acts as a top-level header.

#include <tdslite/tdslite.hpp>
#include <tdslite/net/base/network_io_base.hpp>
#include <tdslite/net/arduino/ethernet/tdsl_netimpl_arduino.hpp>

namespace tdsl {
    /**
     * alias type for tdslite driver with arduino ethernet
     * client implementation for networking.
     *
     * The ethernet MUST be initialized with Ethernet.begin()
     * before interacting with instances of this type.
     */
    template <typename ClientTypeT>
    using arduino_driver = driver<net::tdsl_netimpl_arduino<ClientTypeT>>;

} // namespace tdsl

#endif