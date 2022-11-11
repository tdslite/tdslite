/**
 * _________________________________________________
 *
 * @file   tdsl_string_view.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   23.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSL_UTIL_STRING_VIEW_HPP
#define TDSL_UTIL_STRING_VIEW_HPP

#include <tdslite/util/tdsl_span.hpp>

namespace tdsl {
    /**
     * Wide-character (16 bit) string view
     */
    struct wstring_view : public tdsl::span<const char16_t> {

        using ::tdsl::span<const char16_t>::span;

        wstring_view() : span() {}

        template <tdsl::uint32_t N>
        wstring_view(const char (&str) [N]) : span(str, N) {
            // If the string is NUL-terminated, omit the NUL terminator.
            if (N > 1 && str [N - 2] == '\0' && str [N - 1] == '\0') {
                element_count -= 2;
            }
        }
    };

    /**
     * String view
     */
    struct string_view : public tdsl::char_view {
        using ::tdsl::char_view::span;

        string_view() : span() {}

        template <tdsl::uint32_t N>
        string_view(const char (&str) [N]) : span(str, N) {
            // If the string is NUL-terminated, omit the NUL terminator.
            if (N > 0 && str [N - 1] == '\0') {
                element_count -= 1;
            }
        }
    };
} // namespace tdsl

inline tdsl::string_view operator"" _tsv(const char * val, tdsl::size_t len) noexcept {
    return tdsl::string_view{val, static_cast<tdsl::uint32_t>(len)};
}

inline tdsl::wstring_view operator"" _twsv(const char16_t * val, tdsl::size_t len) noexcept {
    return tdsl::wstring_view{val, static_cast<tdsl::uint32_t>(len)};
}

#endif