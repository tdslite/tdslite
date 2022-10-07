/**
 * _________________________________________________
 *
 * @file   tdsl_default_allocator.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_macrodef.hpp>

#ifndef TDSL_DISABLE_DEFAULT_ALLOCATOR

#include <cstdlib> // for default malloc-free

namespace tdsl {

    inline TDSL_NODISCARD void * tdsl_default_malloc(unsigned long amount) {
        return std::malloc(amount);
    }

    inline void tdsl_default_free(void * ptr) {
        return std::free(ptr);
    }

} // namespace tdsl

#else
namespace tdsl {
    TDSL_NORETURN inline void * tdsl_default_malloc(unsigned long) {
        TDSL_ASSERT_MSG(0,
                        "You have disabled the default allocator but did not provided malloc/free."
                        "Call tdslite_malloc_free() function with your malloc/free functions to"
                        "set non-default allocators before using the library.");
        TDSL_UNREACHABLE;
    }

    TDSL_NORETURN inline void tdsl_default_free(void *) {
        TDSL_ASSERT_MSG(0,
                        "You have disabled the default allocator but did not provided malloc/free."
                        "Call tdslite_malloc_free() function with your malloc/free functions to"
                        "set non-default allocators before using the library.");
        TDSL_UNREACHABLE;
    }
} // namespace tdsl

#endif