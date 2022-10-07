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
#include <cassert>

#ifndef TDSL_DETAIL_TDS_MACRODEF_HPP
#define TDSL_DETAIL_TDS_MACRODEF_HPP

/**
 * Mark a line of code as unreachable. If code flow reaches
 * a line marked with TDSL_UNREACHABLE, the program will
 * be terminated.
 */
#define TDSL_UNREACHABLE __builtin_unreachable();

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
#define TDSL_ALIGNED(Alignment) __attribute__((aligned(Alignment)))

/**
 * @brief [intrinsics.gcc] Change visibility of a function.
 *
 * @param Visibility Visibility value of the function. May be "default" or "hidden".
 */
#define TDSL_SYMBOL_VISIBILITY(Visibility) __attribute__((visibility(Visibility)))

/**
 * @brief [intrinsics.gcc] Mark a function as public.
 *
 * This changes function's visibility to `default`.
 */
#define TDSL_SYMBOL_VISIBLE TDSL_SYMBOL_VISIBILITY("default")

/**
 * @brief [intrinsics.gcc] Mark a function as internal.
 *
 * This changes function's visibility to `hidden`.
 */
#define TDSL_SYMBOL_HIDDEN TDSL_SYMBOL_VISIBILITY("hidden")

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
#define TDSL_PACKED __attribute__((packed))

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
#define TDSL_PACKED_ALIGNED(Alignment) __attribute__((packed, aligned(Alignment)))

/**
 * Retrieve the offset of the member @p m on struct @p st
 */
#define TDSL_OFFSETOF(st, m) (reinterpret_cast<unsigned long long>(&((static_cast<st *>(0))->m)))

/**
 * Assert that COND is satisfied
 */
#define TDSL_ASSERT(COND) assert(COND)

/**
 * Assert with message
 */
#define TDSL_ASSERT_MSG(COND, MSG) assert(!(!(COND)) && MSG)

/**
 * Expect that COND is satisfied
 * FIXME: Implement this
 */
#define TDSL_EXPECT(COND) COND

/**
 * Mark a variable as unused
 */
#define TDSL_MAYBE_UNUSED __attribute__(unused)

/**
 * Nodiscard attribute
 *
 */
#define TDSL_NODISCARD __attribute__((warn_unused_result))

#define TDSL_NORETURN __attribute__((noreturn))

/**
 * Move macro
 */
#define TDSL_MOVE(...) static_cast<typename tdsl::traits::remove_reference<decltype(__VA_ARGS__)>::type &&>(__VA_ARGS__)

/**
 * Forward macro
 */
#define TDSL_FORWARD(...) static_cast<decltype(__VA_ARGS__) &&>(__VA_ARGS__)

/**
 * C++11 __cplusplus version value
 */
#define TDSL_CXX11_VER 201103L

/**
 * C++14 __cplusplus version value
 */
#define TDSL_CXX14_VER 201402L

/**
 * C++17 __cplusplus version value
 */
#define TDSL_CXX17_VER 201703L

/**
 * C++20 __cplusplus version value
 */
#define TDSL_CXX20_VER 202002L

#if __cplusplus >= TDSL_CXX14_VER
#define TDSL_CXX14_CONSTEXPR constexpr
#else
#define TDSL_CXX14_CONSTEXPR
#endif

#endif
