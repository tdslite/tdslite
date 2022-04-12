/**
 * _________________________________________________
 * Macro definitions for tds-lite
 *
 * @file   tds_macrodef.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSLITE_DETAIL_TDS_MACRODEF_HPP
#define TDSLITE_DETAIL_TDS_MACRODEF_HPP

/**
 * Mark a line of code as unreachable. If code flow reaches
 * a line marked with TDSLITE_UNREACHABLE, the program will
 * be terminated.
 */
#define TDSLITE_UNREACHABLE __builtin_unreachable();

/**
 * Assert that COND is satisfied
 */
#define TDSLITE_ASSERT(COND)

/**
 * Assert that COND is satisfied (with custom message)
 */
#define TDSLITE_ASSERT_MSG(COND, MSG)

/**
 * C++11 __cplusplus version value
 */
#define TDSLITE_CXX11_VER 201103L

/**
 * C++14 __cplusplus version value
 */
#define TDSLITE_CXX14_VER 201402L

/**
 * C++17 __cplusplus version value
 */
#define TDSLITE_CXX17_VER 201703L

/**
 * C++20 __cplusplus version value
 */
#define TDSLITE_CXX20_VER 202002L

#if __cplusplus >= TDSLITE_CXX14_VER
#define TDSLITE_CXX14_CONSTEXPR constexpr
#else
#define TDSLITE_CXX14_CONSTEXPR
#endif

#endif
