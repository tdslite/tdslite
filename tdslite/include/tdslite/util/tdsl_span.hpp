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

#ifndef TDSL_DETAIL_TDS_SPAN_HPP
#define TDSL_DETAIL_TDS_SPAN_HPP

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>

#include <span>

namespace tdsl {

    namespace detail {
        template <typename T, typename Q>
        inline static auto exchange(T & dst, Q v) noexcept -> T {
            T prev = dst;
            dst    = v;
            return prev;
        }
    } // namespace detail

    /**
     * Span of T
     */
    template <typename T = const tdsl::uint8_t>
    struct span {

        using element_type    = T;
        using value_type      = typename traits::remove_cv<T>::type;
        using size_type       = tdsl::uint32_t;
        using pointer         = T *;
        using const_pointer   = const T *;
        using reference       = T &;
        using const_reference = const T &;
        using iterator        = pointer;
        using const_iterator  = const_pointer;
        using self_type       = span<element_type>;

        /**
         * Default c-tor
         */
        inline TDSL_CXX14_CONSTEXPR span() noexcept : data_(nullptr), element_count(0){};

        /**
         * Construct a new span object
         *
         * @param [in] elem Pointer to the starting element
         * @param [in] elem_count Element count
         */
        inline explicit constexpr span(element_type * elem, size_type elem_count) noexcept :
            data_(elem), element_count(elem_count) {}

        /**
         * Construct a new span object from a fixed-size
         * array.
         *
         * @tparam N Auto-deduced length of the array
         * @param [in] buffer Fixed-size buffer
         */
        template <size_type N>
        inline constexpr span(element_type (&buffer) [N]) noexcept :
            data_(buffer), element_count(N) {}

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
        inline explicit constexpr span(element_type * begin, element_type * end) noexcept :
            data_(begin), element_count(end - begin) {}

        /**
         * Copy c-tor
         */
        inline constexpr span(const self_type & other) noexcept :
            data_(other.data_), element_count(other.element_count) {}

        /**
         * Move c-tor
         */
        inline constexpr span(self_type && other) noexcept :
            data_(detail::exchange(other.data_, nullptr)),
            element_count(detail::exchange(other.element_count, 0)) {}

        /**
         * Copy assignment
         */
        inline TDSL_CXX14_CONSTEXPR span & operator=(const self_type & other) noexcept {
            // For avoiding "compound-statement in `constexpr` function warning in C++11 mode"
            &other != this ? (data_ = other.data_) : static_cast<element_type *>(nullptr);
            &other != this ? (element_count = other.element_count)
                           : static_cast<decltype(element_count)>(0);
            return *this;
        }

        /**
         * Move assignment
         */
        inline TDSL_CXX14_CONSTEXPR span & operator=(self_type && other) noexcept {
            // For avoiding "compound-statement in `constexpr` function warning in C++11 mode"
            &other != this ? (data_ = detail::exchange(other.data_, nullptr))
                           : static_cast<element_type *>(0);
            &other != this ? (element_count = detail::exchange(other.element_count, 0))
                           : static_cast<decltype(element_count)>(0);
            return *this;
        }

        /**
         * D-tor
         */
        inline ~span() noexcept = default;

        /**
         * Equality comparison operator
         *
         * @param [in] other Span to compare against
         * @return true Spans are equal
         * @return false otherwise
         */
        inline constexpr bool operator==(const self_type & other) const noexcept {
            return data_ == other.data_ && element_count == other.element_count;
        }

        /**
         * Get the amount of the bytes in the span
         *
         * @return size_type Amount of the bytes in the span
         */
        inline constexpr auto size_bytes() const noexcept -> size_type {
            return element_count * sizeof(element_type);
        }

        /**
         * Get the amount of the elements in the span
         *
         * @return size_type Amount of the element in the span
         */
        inline constexpr auto size() const noexcept -> size_type {
            return element_count;
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
            return (data_ + element_count);
        }

        /**
         * Iterator interface begin() function
         *
         * @return T* Pointer to the beginning of the span
         */
        inline constexpr auto cbegin() const noexcept -> const_iterator {
            return data_;
        }

        /**
         * Iterator interface end() function
         *
         * @return T* Pointer to the one past last element of the span
         */
        inline constexpr auto cend() const noexcept -> const_iterator {
            return (data_ + element_count);
        }

        /**
         * Subscript operator
         *
         * @param [in] index Element to access
         * @return reference Reference to the element
         */
        inline auto operator[](size_type index) const noexcept -> reference {
            TDSL_ASSERT_MSG(index < size(), "Array subscript index exceeds span bounds!");
            return data() [index];
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
            return data_ && element_count;
        }

        /**
         * Cast span to another span with new element type
         *
         * @tparam Q New element type
         * @return span<const Q> New span
         *
         * @note If the original span's size_bytes() is not a multiple of sizeof(Q),
         * the element count of the resulting span will be rounded down.
         */
        template <typename Q, traits::enable_when::integral<Q> = true>
        inline constexpr auto rebind_cast() const noexcept -> span<const Q> {
            using rebound_span_type = span<const Q>;
            return rebound_span_type{
                reinterpret_cast<typename rebound_span_type::const_pointer>(data()),
                static_cast<typename rebound_span_type::size_type>(size_bytes() / sizeof(Q))};
        }

    private:
        pointer data_{nullptr}; /**< Pointer to the beginning of the span */
    protected:
        size_type element_count{0}; /**< Size of the span (in element count) */

    }; // struct span

    using char_span    = tdsl::span<const char>;
    using u16char_span = tdsl::span<const char16_t>;
    using u32char_span = tdsl::span<const char32_t>;

} // namespace tdsl

#endif