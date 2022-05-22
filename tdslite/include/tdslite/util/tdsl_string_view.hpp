/**
 * _________________________________________________
 *
 * @file   tdsl_string_view.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   22.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl {
    using char_span      = tdsl::span<const char>;
    using u16char_span   = tdsl::span<const char16_t>;
    using u32string_view = tdsl::span<const char32_t>;
} // namespace tdsl