/**
 * _________________________________________________
 *
 * @file   tdsl_debug_print.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   22.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <cstdio>
#include <cwchar>

#define TDSL_DEBUG_PRINT_ENABLED

#ifdef TDSL_DEBUG_PRINT_ENABLED
#include <tdslite/util/tdsl_hex_dump.hpp>

#define TDSL_DEBUG_PRINT(...)          std::fprintf(stdout, __VA_ARGS__)
#define TDSL_DEBUG_PRINTLN(...)        std::fprintf(stdout, __VA_ARGS__), std::fprintf(stdout, "\n")
#define TDSL_DEBUG_HEXDUMP(ARR, SIZE)  tdsl::util::hexdump(ARR, SIZE)
#define TDSL_DEBUG_HEXPRINT(ARR, SIZE) tdsl::util::hexprint(ARR, SIZE)
#define TDSL_DEBUG_WPRINT(...)         std::wprintf(__VA_ARGS__)
#define TDSL_DEBUG_WPRINTLN(...)       std::wprintf(__VA_ARGS__), std::wprintf("\n")
#define TDSL_DEBUG_PRINT_U16_AS_MB(U16SPAN)                                                                                                \
    for (unsigned int i = 0; i < U16SPAN.size(); i++) {                                                                                    \
        putchar(*reinterpret_cast<const char *>(U16SPAN.data() + i));                                                                      \
    }
#else
#define TDSL_DEBUG_PRINT(...)
#define TDSL_DEBUG_HEXDUMP(...)
#define TDSL_DEBUG_WPRINT(...)
#define TDSL_DEBUG_PRINT_U16_AS_MB(U16SPAN)
#endif