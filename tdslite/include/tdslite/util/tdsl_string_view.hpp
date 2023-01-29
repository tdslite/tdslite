/**
 * _________________________________________________
 * Immutable view types for strings.
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
     *
     * If given character array is nul-terminated, the span
     * will omit the nul terminator from span range.
     */
    struct wstring_view : public tdsl::u16char_view {

        using base_type = tdsl::u16char_view;
        // this does not inherit copy&move constructors
        // so we have to do it ourselves.
        using base_type::span;

        wstring_view(const base_type & other) : base_type::span(other) {}

        wstring_view(base_type && other) : base_type::span(other) {}

        wstring_view() : span() {}

        // byte constructor
        wstring_view(tdsl::byte_view bv) : span(bv.rebind_cast<char16_t>()) {
            // If the string is NUL-terminated, omit the NUL terminator.

            if (bv.size() > 1 && bv [bv.size() - 2] == '\0' && bv [bv.size() - 1] == '\0') {
                element_count -= 1;
            }
        }

        template <tdsl::uint32_t N>
        wstring_view(const char16_t (&str) [N]) : span(str, N) {
            // If the string is NUL-terminated, omit the NUL terminator.
            if (N > 0 && str [N - 1] == u'\0') {
                element_count -= 1;
            }
        }
    };

    /**
     * String view
     *
     * If given character array is nul-terminated, the span
     * will omit the nul terminator from span range.
     */
    struct string_view : public tdsl::char_view {

        using base_type = tdsl::char_view;
        // this does not inherit copy&move constructors
        // so we have to do it ourselves.
        using base_type::span;

        string_view(const base_type & other) : base_type::span(other) {}

        string_view(base_type && other) : base_type::span(other) {}

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

#if defined(PROGMEM) && !defined(TDSL_FORCE_DISABLE_PROGMEM_STRING_VIEW)

#if !defined(__AVR__) && defined(memcpy_P)

#undef memcpy_P

inline void * memcpy_P(void * dest, const void * src, size_t n) noexcept {
    return memcpy(dest, src, n);
}

#endif

#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>

namespace tdsl {
    struct progmem_forward_iterator;
} // namespace tdsl

bool operator==(const tdsl::progmem_forward_iterator & a,
                const tdsl::progmem_forward_iterator & b) noexcept;
bool operator!=(const tdsl::progmem_forward_iterator & a,
                const tdsl::progmem_forward_iterator & b) noexcept;

// pgm_read_byte_near

template <class... Args>
struct tdsl_memcpyp_detect {

    template <class... Argsv>
    struct type_holder {};

    template <class T, class = void>
    struct x : tdsl::traits::false_type {};

    template <class... Xargs>
    struct x<type_holder<Xargs...>, tdsl::traits::void_t<decltype(memcpy_P(
                                        static_cast<Xargs>(tdsl::traits::declval<Xargs>())...))>>
        : tdsl::traits::true_type {};

    static constexpr bool value = x<type_holder<Args...>>::value;
};

namespace tdsl {

    struct progmem_forward_iterator {

        inline progmem_forward_iterator(tdsl::char_view span) noexcept :
            reader{span.rebind_cast<const tdsl::uint8_t>()} {
            static_assert(
                traits::dependent_bool<
                    tdsl_memcpyp_detect<void *, const void *, tdsl::size_t>::value>::value,
                "The progmem_forward_iterator type requires memcpy_P function to be available!");
        }

        /**
         * Return the char at current iterator
         * position.
         *
         * @return Char value read from program memory
         */
        inline char operator*() const {
            // binary_reader uses memcpy to read,by default
            // so it is sufficient to override the memcpy
            // to read from program memory space instead of
            // SRAM.
            return reader.peek_raw<char>(::memcpy_P);
        }

        /**
         * Advance iterator forward (pre-increment)
         */
        inline progmem_forward_iterator & operator++() noexcept {
            bool b = reader.advance(sizeof(char));
            (void) b;
            TDSL_ASSERT_MSG(b, "Something is wrong (attempted OOB advance)");
            return *this;
        }

        /**
         * Advance iterator forward (post-increment)
         */
        inline progmem_forward_iterator & operator++(int) noexcept {
            bool b = reader.advance(sizeof(char));
            (void) b;
            TDSL_ASSERT_MSG(b, "Something is wrong (attempted OOB advance)");
            return *this;
        }

        friend bool(::operator==)(const progmem_forward_iterator &,
                                  const progmem_forward_iterator &) noexcept;

        friend bool(::operator!=)(const progmem_forward_iterator & a,
                                  const progmem_forward_iterator & b) noexcept;

    private:
        tdsl::binary_reader<tdsl::endian::native> reader;
    };

    /**
     * An helper type that allows transparent iterative reads
     * on strings which are stored in the program memory.
     *
     * Program memory strings are declared with PROGMEM macro and
     * they reside in a separate flash memory block named program
     * memory. This memory block is isolated from the main SRAM.
     *
     */
    struct progmem_string_view : private tdsl::char_view {

        using tdsl::char_view::size;
        using tdsl::char_view::size_bytes;
        using tdsl::char_view::operator bool;
        using element_type = typename tdsl::char_view::element_type;

        progmem_string_view() : tdsl::char_view() {}

        /**
         * Construct a new progmem string view object
         */
        template <tdsl::uint32_t N>
        inline progmem_string_view(const char (&str) [N]) : span(str, N) {

            // If the string is NUL-terminated, omit the NUL terminator.
            if (N > 0 && operator[](N - 1) == '\0') {
                element_count -= 1;
            }
        }
#if defined pgm_read_byte_near
        /**
         * Array subscript operator
         *
         * @param [in] idx Index of the element to read
         * @return char at position @p idx
         */
        inline auto operator[](int idx) const noexcept -> char {
            // static_assert(
            //     traits::dependent_bool<tdsl_pgm_read_byte_near_detect<const char
            //     *>::value>::value, "The progmem_string_view type requires pgm_read_byte_near
            //     function to be " "available!");
            return pgm_read_byte_near(tdsl::char_view::begin() + idx);
        }
#else
#error "pgm_read_byte_near macro is undefined!"
#endif

        /**
         * Get an iterator to the beginning of the
         * underlying progmem string.
         *
         * @return progmem_forward_iterator The iterator
         */
        inline progmem_forward_iterator begin() const noexcept {
            return progmem_forward_iterator{*this};
        }

        /**
         * Get an iterator to the end of the
         * underlying progmem string.
         *
         * @return progmem_forward_iterator The iterator
         */
        inline progmem_forward_iterator end() const noexcept {
            return progmem_forward_iterator{
                tdsl::char_view{tdsl::char_view::end(), tdsl::char_view::end()}
            };
        }

        /**
         * Get raw data pointer
         */
        inline const char * raw_data() const noexcept {
            return data();
        }
    };

} // namespace tdsl

/**
 * Equality operator
 *
 * @param [in] a Iterator a
 * @param [in] b Iterator b
 * @return true if a is pointing to same memory location with b
 * @return false otherwise
 */
inline bool operator==(const tdsl::progmem_forward_iterator & a,
                       const tdsl::progmem_forward_iterator & b) noexcept {

    return a.reader.current() == b.reader.current();
};

/**
 * Inequality operator
 *
 * @param [in] a Iterator a
 * @param [in] b Iterator b
 * @return true if a is NOT pointing to same memory location with b
 * @return false otherwise
 */
inline bool operator!=(const tdsl::progmem_forward_iterator & a,
                       const tdsl::progmem_forward_iterator & b) noexcept {
    return !operator==(a, b);
};

/**
 * Auto-wraps and stores a string literal in
 * PROGMEM. Returns a tdsl::progmem_string_view
 * to created progmem string.
 *
 * @param [in] X The string literal
 */
#define TDSL_PMEMSTR(X)                                                                            \
    tdsl::progmem_string_view {                                                                    \
        [&]() -> decltype(X) {                                                                     \
            static const char __pm [] PROGMEM = X;                                                 \
            return __pm;                                                                           \
        }()                                                                                        \
    }

#endif

inline tdsl::string_view operator"" _tsv(const char * val, tdsl::size_t len) noexcept {
    return tdsl::string_view{val, len};
}

inline tdsl::wstring_view operator"" _twsv(const char16_t * val, tdsl::size_t len) noexcept {
    return tdsl::wstring_view{val, len};
}

#endif