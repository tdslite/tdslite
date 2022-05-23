/**
 * _________________________________________________
 *
 * @file   tdsl_string_writer.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   23.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_string_view.hpp>

namespace tdsl { namespace detail {

    /**
     * Helper type for writing string parameters into a TDS packet.
     *
     * The string parameters are handled depending on source string type
     * (i.e. single-char string, wide (utf-16) string)).
     *
     * @tparam TDSCTX TDS context type
     */
    template <typename TDSCTX>
    struct string_parameter_writer {

        static void write(typename TDSCTX::xmit_context & xc, const string_view & sv,
                          void (*encoder)(tdsl::uint8_t *, tdsl::uint32_t) = nullptr) {
            for (auto ch : sv) {
                char16_t c = ch;
                if (encoder) {
                    encoder(reinterpret_cast<tdsl::uint8_t *>(&c), sizeof(char16_t));
                }

                xc.write(c);
            }
        }

        static void write(typename TDSCTX::xmit_context & xc, const wstring_view & sv,
                          void (*encoder)(tdsl::uint8_t *, tdsl::uint32_t) = nullptr) {

            if (not encoder) {
                xc.write(sv);
                return;
            }

            // Password needs special treatment before sending.
            for (auto ch : sv) {
                char16_t c = ch;
                encoder(reinterpret_cast<tdsl::uint8_t *>(&c), sizeof(char16_t));
                xc.write(c);
            }
        }

        static inline auto calculate_write_size(const wstring_view & sv) noexcept -> tdsl::uint32_t {
            return sv.size_bytes();
        }

        static inline auto calculate_write_size(const string_view & sv) noexcept -> tdsl::uint32_t {
            return sv.size_bytes() * sizeof(char16_t);
        }
    };
}} // namespace tdsl::detail