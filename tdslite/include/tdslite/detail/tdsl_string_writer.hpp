/**
 * ____________________________________________________
 * Helper type for writing strings with different
 * character sizes transparently to a TDS message.
 *
 * @file   tdsl_string_writer.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   23.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_STRING_PARAMETER_WRITER_HPP
#define TDSL_DETAIL_STRING_PARAMETER_WRITER_HPP

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_string_view.hpp>

namespace tdsl { namespace detail {

    /**
     * Helper type for writing string parameters into a TDS packet.
     *
     * The string parameters are handled depending on source string type
     * (i.e. single-char string, wide (utf-16) string)).
     *
     * ASCII strings are expanded into UTF-16 (wide) strings while
     * writing.
     *
     * @tparam TDSCTX TDS context type
     */
    template <typename TDSCTX>
    struct string_parameter_writer {

        struct counted_writer {
            counted_writer(TDSCTX & tds_ctx) : tds_ctx(tds_ctx) {}

            // --------------------------------------------------------------------------------

            inline void write(const tdsl::string_view & sv) noexcept {
                string_parameter_writer<TDSCTX>::write(tds_ctx, sv);
                written_chars += string_parameter_writer<TDSCTX>::calculate_write_size(sv);
            }

            // --------------------------------------------------------------------------------

            inline TDSL_NODISCARD auto get() const noexcept -> tdsl::size_t {
                return written_chars;
            }

        private:
            TDSCTX & tds_ctx;
            tdsl::size_t written_chars = {0};
        };

        // --------------------------------------------------------------------------------

        static inline TDSL_NODISCARD auto make_counted_writer(TDSCTX & tds_ctx) noexcept
            -> counted_writer {
            return {tds_ctx};
        }

        // --------------------------------------------------------------------------------

        static inline void write(typename TDSCTX::tx_mixin & xc, const string_view & sv,
                                 void (*encoder)(tdsl::uint8_t *,
                                                 tdsl::uint32_t) = nullptr) noexcept {
            for (auto ch : sv) {
                char16_t c = ch;
                if (encoder) {
                    encoder(reinterpret_cast<tdsl::uint8_t *>(&c), sizeof(char16_t));
                }
                xc.write(c);
            }
        }

        // --------------------------------------------------------------------------------

        static inline void write(typename TDSCTX::rx_mixin & xc, const wstring_view & sv,
                                 void (*encoder)(tdsl::uint8_t *,
                                                 tdsl::uint32_t) = nullptr) noexcept {

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

        // --------------------------------------------------------------------------------

        template <typename T = struct progmem_string_view>
        static inline void write(typename TDSCTX::tx_mixin & xc, const T & sv,
                                 void (*encoder)(tdsl::uint8_t *, tdsl::uint32_t) = nullptr) {
            for (auto ch : sv) {
                char16_t c = ch;
                if (encoder) {
                    encoder(reinterpret_cast<tdsl::uint8_t *>(&c), sizeof(char16_t));
                }
                xc.write(c);
            }
        }

        // --------------------------------------------------------------------------------

        template <typename T>
        static inline auto calculate_write_size(const T & sv) noexcept -> tdsl::size_t {
            return sv.size_bytes() * (sizeof(char16_t) / sizeof(typename T::element_type));
        }
    };
}} // namespace tdsl::detail

#endif