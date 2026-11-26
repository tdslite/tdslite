/**
 * ____________________________________________________
 * The default memory allocation functions for tdslite.
 *
 * @file   tdsl_default_allocator.hpp
 * @author mkg <me@mustafagilor.com>
 *
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_DEFAULT_ALLOCATOR_HPP
#define TDSL_DETAIL_DEFAULT_ALLOCATOR_HPP

#include <tdslite/util/tdsl_macrodef.hpp>

// Define TDSL_DISABLE_DEFAULT_ALLOCATOR to
// disable default alloc/dealloc functions.
#ifndef TDSL_DISABLE_DEFAULT_ALLOCATOR

#include <stdlib.h> // for default malloc-free

namespace tdsl {

    // --------------------------------------------------------------------------------

    /**
     * Default malloc function.
     *
     * Calls ::malloc internally.
     *
     * @param [in] amount Amount of bytes to allocate
     *
     * Allocated memory must be freed back with @ref tdsl_default_free
     *
     * @returns Non-nullptr to allocated memory on success,
     * @returns nullptr on failure
     */
    inline TDSL_NODISCARD void * tdsl_default_malloc(unsigned long amount) noexcept {
        // ::malloc's size_t may be narrower than unsigned long (e.g. on AVR)
        using platform_size_t = decltype(sizeof(int));
        if (amount > ~platform_size_t{0}) {
            return nullptr;
        }
        return ::malloc(static_cast<platform_size_t>(amount));
    }

    // --------------------------------------------------------------------------------

    /**
     * Default free function
     *
     * Calls ::free internally.
     *
     * @param [in] ptr Pointer to memory to be freed.
     *
     * The memory pointed by @p  must have been allocated by tdsl_default_alloc,
     * otherwise the behavior of this function will be undefined.
     */
    inline void tdsl_default_free(void * ptr) noexcept {
        return ::free(ptr);
    }

} // namespace tdsl

#else
namespace tdsl {

    // --------------------------------------------------------------------------------

    TDSL_NORETURN inline void * tdsl_default_malloc(unsigned long) noexcept {
        TDSL_ASSERT_MSG(0,
                        "You have disabled the default allocator but did not provided malloc/free."
                        "Call tdslite_malloc_free() function with your malloc/free functions to"
                        "set non-default allocators before using the library.");
        TDSL_UNREACHABLE;
    }

    // --------------------------------------------------------------------------------

    TDSL_NORETURN inline void tdsl_default_free(void *) noexcept {
        TDSL_ASSERT_MSG(0,
                        "You have disabled the default allocator but did not provided malloc/free."
                        "Call tdslite_malloc_free() function with your malloc/free functions to"
                        "set non-default allocators before using the library.");
        TDSL_UNREACHABLE;
    }
} // namespace tdsl

#endif

#endif