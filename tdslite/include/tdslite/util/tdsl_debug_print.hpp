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

#define TDSLITE_DEBUG_PRINT_ENABLED

#ifdef TDSLITE_DEBUG_PRINT_ENABLED
#include <tdslite/util/tdsl_hex_dump.hpp>

#define TDSLITE_DEBUG_PRINT(...)         std::fprintf(stdout, __VA_ARGS__)
#define TDSLITE_DEBUG_HEXDUMP(ARR, SIZE) tdsl::util::hexdump(ARR, SIZE)
#define TDSLITE_DEBUG_WPRINT(...)        std::wprintf(__VA_ARGS__)
#define TDSLITE_DEBUG_PRINT_U16_AS_MB(U16SPAN)                                                                                             \
    for (unsigned int i = 0; i < U16SPAN.size(); i++) {                                                                                    \
        putchar(*reinterpret_cast<const char *>(U16SPAN.data() + i));                                                                      \
    }
#else
#define TDSLITE_DEBUG_PRINT(...)
#define TDSLITE_DEBUG_HEXDUMP(...)
#define TDSLITE_DEBUG_WPRINT(...)
#define TDSLITE_DEBUG_PRINT_U16_AS_MB(U16SPAN)
#endif