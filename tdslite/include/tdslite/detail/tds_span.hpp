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
    template <typename T = const tdslite::uint8_t>
    struct span {

        /**
         */
        span()           = delete;

        /**
         * Destructor
         */
        ~span() noexcept = default;

        inline constexpr span(const span<T> & other) noexcept : data(other.data), size(other.size) {}

        /**
         * Move constructor
         */
        inline constexpr span(span<T> && other) noexcept : data(other.data), size(other.size) {}

        /**
         * Copy assignment
         */
        inline constexpr span & operator=(const span<T> & other) noexcept {
            if (&other != this) {
                data = other.data;
                size = other.size;
            }
            return *this;
        }

        /**
         * Move assignment
         */
        inline constexpr span & operator=(span<T> && other) noexcept {
            if (&other != this) {
                data       = other.data;
                size       = other.size;
                other.data = nullptr;
                other.size = 0;
            }
            return *this;
        }

        /**
         * Construct a new span object
         *
         * @param [in] data
         * @param [in] size
         */
        inline explicit constexpr span(T * data, tdslite::uint32_t size) noexcept : data(data), size(size) {}

        /**
         * Construct a new span object from a fixed-size
         * array.
         *
         * @tparam N Auto-deduced length of the array
         * @param [in] buffer Fixed-size buffer
         */
        template <tdslite::uint32_t N>
        inline explicit constexpr span(T (&buffer) [N]) noexcept : data(buffer), size(N) {}

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
        inline explicit constexpr span(T * begin, const T * end) noexcept : data(begin), size(end - begin) {}

        /**
         * Get the amount of the bytes in the span
         *
         * @return tdslite::uint32_t Amount of the bytes in the span
         */
        inline constexpr auto size_bytes() const noexcept -> tdslite::uint32_t {
            return size;
        }

        /**
         * Iterator interface begin() function
         *
         * @return T* Pointer to the beginning of the span
         */
        inline constexpr auto begin() const noexcept -> T * {
            return data;
        }

        /**
         * Iterator interface end() function
         *
         * @return T* Pointer to the one past last element of the span
         */
        inline constexpr auto end() const noexcept -> T * {
            return (data + size);
        }

        /**
         * Make span boolable, so it can be used expressions like:
         * span<int> span_v;
         * ...
         * if(span_v){
         *   ..do stuff..
         * }
         *
         * @returns true if data is not nullptr and size is greater than zero
         * @returns false otherwise
         */
        inline operator bool() const noexcept {
            return data && size;
        }

        T * data{nullptr};               /**< Pointer to the beginning of the span */
        const tdslite::uint32_t size{0}; /**< Size of the span (in bytes) */

    }; // struct span

} // namespace tdslite

#endif