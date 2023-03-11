/**
 * ____________________________________________________
 * Utility functions for printing out buffers in human
 * readable hex format.
 *
 * @file   tdsl_hex_dump.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   20.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_UTIL_HEX_DUMP_HPP
#define TDSL_UTIL_HEX_DUMP_HPP

#include <tdslite/util/tdsl_span.hpp>

#include <stdio.h>
#include <ctype.h>

namespace tdsl { namespace util {

    // --------------------------------------------------------------------------------

    /**
     * Dump a byte buffer as hex to the console
     *
     * @param [in] ptr Pointer to the buffer
     * @param [in] buflen Buffer's length
     */
    static inline void hexdump(const void * ptr, unsigned long long buflen) noexcept {
        const unsigned char * buf = static_cast<const unsigned char *>(ptr);
        for (unsigned long i = 0; i < buflen; i += 16) {
            printf("%06lx: ", i);
            for (unsigned long j = 0; j < 16; j++)
                if (i + j < buflen)
                    printf("%02x ", buf [i + j]);
                else
                    printf("   ");
            printf(" ");
            for (unsigned long j = 0; j < 16; j++)
                if (i + j < buflen)
                    printf("%c", isprint(buf [i + j]) ? buf [i + j] : '.');
            printf("\n");
        }
    }

    // --------------------------------------------------------------------------------

    static inline void hexdump(tdsl::byte_view v) {
        return hexdump(v.data(), v.size_bytes());
    }

    // --------------------------------------------------------------------------------

    static inline void hexprint(const void * ptr, unsigned long long buflen) noexcept {
        const unsigned char * buf = static_cast<const unsigned char *>(ptr);
        for (unsigned long long i = 0; i < buflen; i++) {
            printf("%02x ", buf [i]);
        }

        printf(" ");
        for (unsigned long long i = 0; i < buflen; i++) {
            printf("%c", isprint(buf [i]) ? buf [i] : '.');
        }
    }

    // --------------------------------------------------------------------------------

    static inline void hexprint(tdsl::byte_view v) {
        return hexprint(v.data(), v.size_bytes());
    }
}} // namespace tdsl::util

#endif