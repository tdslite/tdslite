/**
 * ____________________________________________________
 * Span implementation
 *
 * @file   tds_span.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSLITE_DETAIL_TDS_SPAN_HPP
#define TDSLITE_DETAIL_TDS_SPAN_HPP

#include <tdslite/detail/tds_inttypes.hpp>

namespace tdslite {

    /**
     * Span of bytes
     */
    struct span {

        /**
         * Default constructor (deleted)
         */
        span() = delete;

        /**
         * Construct a new span object
         *
         * @param [in] data
         * @param [in] size
         */
        explicit constexpr span(const tdslite::uint8_t * data, tdslite::uint32_t size) noexcept : data(data), size(size) {}

        /**
         * Construct a new span object from a fixed-size
         * array.
         *
         * @tparam N Auto-deduced length of the array
         * @param [in] buffer Fixed-size buffer
         */
        template <tdslite::uint32_t N>
        explicit constexpr span(const tdslite::uint8_t (&buffer) [N]) noexcept : data(buffer), size(N) {}

        /**
         * Construct a new span object from explicit begin
         * and end parameters
         *
         * @param [in] begin Start of the span.
         * @param [in] end End of the span. Must be greater than @p begin
         *
         * @note @p end must point to one past last element
         * @note @p start must be always lesser than @p end
         * @note @p end must be always greater than @p start
         */
        template <tdslite::uint32_t N>
        explicit constexpr span(const tdslite::uint8_t * begin, const tdslite::uint8_t * end) noexcept : data(begin), size(end - begin) {}

        const tdslite::uint8_t * data; /**< Pointer to the beginning of the span */
        const tdslite::uint32_t size;  /**< Size of the span (in bytes) */

    }; // struct span

} // namespace tdslite

#endif