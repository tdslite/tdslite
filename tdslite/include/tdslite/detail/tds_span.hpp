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
#include <tdslite/detail/tds_macrodef.hpp>
#include <tdslite/detail/tds_type_traits.hpp>

namespace tdslite {

    namespace detail {
        template <typename T, typename Q>
        inline static auto exchange(T & dst, Q v) noexcept -> T {
            T prev = dst;
            dst    = v;
            return prev;
        }
    } // namespace detail

    /**
     * Span of bytes
     */
    template <typename T = const tdslite::uint8_t>
    struct span {

        using element_type    = T;
        using value_type      = typename traits::remove_cv<T>::type;
        using size_type       = tdslite::uint32_t;
        using pointer         = T *;
        using const_pointer   = const T *;
        using reference       = T &;
        using const_reference = const T &;
        using iterator        = pointer;
        using const_iterator  = const_pointer;
        using self_type       = span<element_type>;

        /**
         */
        inline TDSLITE_CXX14_CONSTEXPR span() noexcept : data_(nullptr), size_(0){};

        /**
         * Destructor
         */
        inline ~span() noexcept = default;

        inline constexpr span(const self_type & other) noexcept : data_(other.data_), size_(other.size_) {}

        /**
         * Move constructor
         */
        inline constexpr span(self_type && other) noexcept :
            data_(detail::exchange(other.data_, nullptr)), size_(detail::exchange(other.size_, 0)) {}

        /**
         * Copy assignment
         */
        inline TDSLITE_CXX14_CONSTEXPR span & operator=(const self_type & other) noexcept {
            // For avoiding "compound-statement in `constexpr` function warning in C++11 mode"
            &other != this ? (data_ = other.data_) : static_cast<element_type *>(nullptr);
            &other != this ? (size_ = other.size_) : static_cast<decltype(size_)>(0);
            return *this;
        }

        /**
         * Move assignment
         */
        inline TDSLITE_CXX14_CONSTEXPR span & operator=(self_type && other) noexcept {
            // For avoiding "compound-statement in `constexpr` function warning in C++11 mode"
            &other != this ? (data_ = detail::exchange(other.data_, nullptr)) : static_cast<element_type *>(0);
            &other != this ? (size_ = detail::exchange(other.size_, 0)) : static_cast<decltype(size_)>(0);
            return *this;
        }

        /**
         * Equality comparison operator
         *
         * @param [in] other Span to compare against
         * @return true Spans are equal
         * @return false otherwise
         */
        inline constexpr bool operator==(const self_type & other) const noexcept {
            return data_ == other.data_ && size_ == other.size_;
        }

        /**
         * Construct a new span object
         *
         * @param [in] data Pointer to the data
         * @param [in] size Size of the data
         */
        inline explicit constexpr span(element_type * data, size_type size) noexcept : data_(data), size_(size) {}

        /**
         * Construct a new span object from a fixed-size
         * array.
         *
         * @tparam N Auto-deduced length of the array
         * @param [in] buffer Fixed-size buffer
         */
        template <size_type N>
        inline constexpr span(element_type (&buffer) [N]) noexcept : data_(buffer), size_(N) {}

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
        inline explicit constexpr span(element_type * begin, element_type * end) noexcept : data_(begin), size_(end - begin) {}

        /**
         * Get the amount of the bytes in the span
         *
         * @return size_type Amount of the bytes in the span
         */
        inline constexpr auto size_bytes() const noexcept -> size_type {
            return size_;
        }

        /**
         * Data pointer
         *
         * @return T* Pointer to the beginning of the span
         */
        inline constexpr auto data() const noexcept -> pointer {
            return data_;
        }

        /**
         * Iterator interface begin() function
         *
         * @return T* Pointer to the beginning of the span
         */
        inline constexpr auto begin() const noexcept -> iterator {
            return data_;
        }

        /**
         * Iterator interface end() function
         *
         * @return T* Pointer to the one past last element of the span
         */
        inline constexpr auto end() const noexcept -> iterator {
            return (data_ + size_);
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
            return data_ && size_;
        }

    private:
        pointer data_{nullptr}; /**< Pointer to the beginning of the span */
    protected:
        size_type size_{0}; /**< Size of the span (in bytes) */

    }; // struct span

} // namespace tdslite

#endif