/**
 * _________________________________________________
 * Unsigned to string conversion utilitites
 *
 * @file   tdsl_utos.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   20.01.2023
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSL_UTIL_UTOS_HPP
#define TDSL_UTIL_UTOS_HPP

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_span.hpp>

namespace tdsl {

    /**
     * Write string representation of @p val to @p out
     *
     * The caller is responsible for making sure @p out has enough
     * space to hold the string representation. Otherwise, the result
     * might be trimmed. The output is not NUL terminated.
     *
     * @param [in] val Value
     * @param [in] out Output char span
     *
     * @note `utos` stands for `unsigned to string`.
     *
     * @return tdsl::char_view A subspan of @p out contains
     *         the string representation
     */
    TDSL_NODISCARD static inline tdsl::char_view utos(tdsl::size_t val,
                                                      tdsl::char_span out) noexcept {
        if (not out.size_bytes()) {
            return tdsl::char_view{};
        }

        tdsl::ssize_t i = (static_cast<tdsl::ssize_t>(out.size_bytes()) - 1);

        // Edge case where val is 0
        if (val == 0) {
            out [i--] = '0';
        }
        else {
            for (; val && i >= 0; --i, val /= 10) {
                out [i] = static_cast<char>(static_cast<char>(val % 10) + '0');
            }
        }

        return tdsl::char_view{&out [i + 1], out.end()};
    };

} // namespace tdsl

#endif