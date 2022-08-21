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

#include <tdslite/util/tdsl_type_traits.hpp>

#ifndef TDSLITE_DETAIL_TDS_MACRODEF_HPP
#define TDSLITE_DETAIL_TDS_MACRODEF_HPP

/**
 * Mark a line of code as unreachable. If code flow reaches
 * a line marked with TDSLITE_UNREACHABLE, the program will
 * be terminated.
 */
#define TDSLITE_UNREACHABLE __builtin_unreachable();

/**
 * @brief [intrinsics.gcc] Specify minimum alignment for given type. This causes target type to
 * be allocated and aligned at least Alignment bytes boundary.
 *
 * @note The `aligned` attribute can only increase alignment boundary. To decrease, see `packed` attribute as well.
 *
 * @note Maximum alignment may depend on your linker.
 *
 * @param Alignment Alignment byte boundary
 */
#define TDSLITE_ALIGNED(Alignment) __attribute__((aligned(Alignment)))

/**
 * @brief [intrinsics.gcc] Change visibility of a function.
 *
 * @param Visibility Visibility value of the function. May be "default" or "hidden".
 */
#define TDSLITE_SYMBOL_VISIBILITY(Visibility) __attribute__((visibility(Visibility)))

/**
 * @brief [intrinsics.gcc] Mark a function as public.
 *
 * This changes function's visibility to `default`.
 */
#define TDSLITE_SYMBOL_VISIBLE TDSLITE_SYMBOL_VISIBILITY("default")

/**
 * @brief [intrinsics.gcc] Mark a function as internal.
 *
 * This changes function's visibility to `hidden`.
 */
#define TDSLITE_SYMBOL_HIDDEN TDSLITE_SYMBOL_VISIBILITY("hidden")

/**
 * @brief [intrinsics.gcc] Pack a struct or union type to layout which allows smallest possible space.
 *
 * This prevents compiler to pad variables inside struct or union type to largest possible
 * type alignment available for target platform.
 *
 * @note Packed types are usually used for mapping transmitted messages directly to data types,
 * or space-limited environments.
 *
 * @warning Taking address of a variable declared inside of a packed type may result in undefined behavior.
 */
#define TDSLITE_PACKED __attribute__((packed))

/**
 * @brief [intrinsics.gcc] Pack and align a type to alignment boundary of X
 *
 * @param Alignment Alignment boundary
 *
 * @note Packed types are usually used for mapping transmitted messages directly to data types,
 * or space-limited environments.
 *
 * @warning Taking address of a variable declared inside of a packed type may result in undefined behavior.
 */
#define TDSLITE_PACKED_ALIGNED(Alignment) __attribute__((packed, aligned(Alignment)))

/**
 * Retrieve the offset of the member @p m on struct @p st
 */
#define TDSLITE_OFFSETOF(st, m) (reinterpret_cast<unsigned long long>(&((static_cast<st *>(0))->m)))

/**
 * Assert that COND is satisfied
 * FIXME: Implement this
 */
#define TDSLITE_ASSERT(COND)

/**
 * Expect that COND is satisfied
 * FIXME: Implement this
 */
#define TDSLITE_EXPECT(COND) COND

/**
 * Assert with message
 * FIXME: Implement this
 */
#define TDSLITE_ASSERT_MSG(COND, MSG)

/**
 * Mark a variable as unused
 */
#define TDSLITE_MAYBE_UNUSED __attribute__(unused)

/**
 * Nodiscard attribute
 *
 */
#define TDSLITE_NODISCARD __attribute__((warn_unused_result))

/**
 * Move macro
 */
#define TDSLITE_MOVE(...) static_cast<tdsl::traits::remove_reference<decltype(__VA_ARGS__)>::type &&>(__VA_ARGS__)

/**
 * Forward macro
 */
#define TDSLITE_FORWARD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)

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
