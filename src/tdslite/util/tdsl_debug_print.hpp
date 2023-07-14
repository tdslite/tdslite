/**
 * ____________________________________________________
 * Debug printing utilities
 *
 * @file   tdsl_debug_print.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   22.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_UTIL_DEBUG_PRINT_HPP
#define TDSL_UTIL_DEBUG_PRINT_HPP

#ifdef TDSL_DEBUG_PRINT_ENABLED
#include <cstdio>
#include <cwchar>
#include <tdslite/util/tdsl_hex_dump.hpp>

#define TDSL_DEBUG_PRINT(...)          std::fprintf(stdout, __VA_ARGS__)
#define TDSL_DEBUG_PRINTLN(...)        std::fprintf(stdout, __VA_ARGS__), std::fprintf(stdout, "\n")
#define TDSL_DEBUG_HEXDUMP(ARR, SIZE)  tdsl::util::hexdump(ARR, SIZE)
#define TDSL_DEBUG_HEXPRINT(ARR, SIZE) tdsl::util::hexprint(ARR, SIZE)
#define TDSL_DEBUG_WPRINT(...)         std::wprintf(__VA_ARGS__)
#define TDSL_DEBUG_WPRINTLN(...)       std::wprintf(__VA_ARGS__), std::wprintf("\n")
#define TDSL_DEBUG_PRINT_U16_AS_MB(U16SPAN)                                                        \
    for (unsigned int i = 0; i < U16SPAN.size(); i++) {                                            \
        putchar(*reinterpret_cast<const char *>(U16SPAN.data() + i));                              \
    }
#else

#ifndef TDSL_DEBUG_PRINT
#define TDSL_DEBUG_PRINT(...)
#endif
#ifndef TDSL_DEBUG_PRINTLN
#define TDSL_DEBUG_PRINTLN(...)
#endif
#ifndef TDSL_DEBUG_HEXDUMP
#define TDSL_DEBUG_HEXDUMP(...)
#endif
#ifndef TDSL_DEBUG_HEXPRINT
#define TDSL_DEBUG_HEXPRINT(ARR, SIZE)
#endif
#ifndef TDSL_DEBUG_WPRINT
#define TDSL_DEBUG_WPRINT(...)
#endif
#ifndef TDSL_DEBUG_WPRINTLN
#define TDSL_DEBUG_WPRINTLN(...)
#endif
#ifndef TDSL_DEBUG_PRINT_U16_AS_MB
#define TDSL_DEBUG_PRINT_U16_AS_MB(U16SPAN)
#endif
#endif

#ifdef TDSL_TRACE_PRINT_ENABLED
#include <cstdio>
#include <cwchar>
#include <tdslite/util/tdsl_hex_dump.hpp>

#define TDSL_TRACE_PRINT(...)          std::fprintf(stdout, __VA_ARGS__)
#define TDSL_TRACE_PRINTLN(...)        std::fprintf(stdout, __VA_ARGS__), std::fprintf(stdout, "\n")
#define TDSL_TRACE_HEXDUMP(ARR, SIZE)  tdsl::util::hexdump(ARR, SIZE)
#define TDSL_TRACE_HEXPRINT(ARR, SIZE) tdsl::util::hexprint(ARR, SIZE)
#define TDSL_TRACE_WPRINT(...)         std::wprintf(__VA_ARGS__)
#define TDSL_TRACE_WPRINTLN(...)       std::wprintf(__VA_ARGS__), std::wprintf("\n")
#define TDSL_TRACE_PRINT_U16_AS_MB(U16SPAN)                                                        \
    for (unsigned int i = 0; i < U16SPAN.size(); i++) {                                            \
        putchar(*reinterpret_cast<const char *>(U16SPAN.data() + i));                              \
    }
#else

#ifndef TDSL_TRACE_PRINT
#define TDSL_TRACE_PRINT(...)
#endif
#ifndef TDSL_TRACE_PRINTLN
#define TDSL_TRACE_PRINTLN(...)
#endif
#ifndef TDSL_TRACE_HEXDUMP
#define TDSL_TRACE_HEXDUMP(...)
#endif
#ifndef TDSL_TRACE_HEXPRINT
#define TDSL_TRACE_HEXPRINT(ARR, SIZE)
#endif
#ifndef TDSL_TRACE_WPRINT
#define TDSL_TRACE_WPRINT(...)
#endif
#ifndef TDSL_TRACE_WPRINTLN
#define TDSL_TRACE_WPRINTLN(...)
#endif
#ifndef TDSL_TRACE_PRINT_U16_AS_MB
#define TDSL_TRACE_PRINT_U16_AS_MB(U16SPAN)
#endif
#endif

#endif