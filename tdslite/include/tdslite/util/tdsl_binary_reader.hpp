/**
 * ____________________________________________________
 * binary_reader<> template class implementation
 *
 * @file   tds_binary_reader.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TDS_BINARY_READER_HPP
#define TDSL_DETAIL_TDS_BINARY_READER_HPP

#include <tdslite/util/tdsl_byte_swap.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_endian.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>

#include <string.h>

namespace tdsl {

    /**
     * A helper class to read from contiguous stream of bytes
     *
     * Reader is endianness-aware, meaning that if host endianness
     * and data endianness are different, reader will swap the byte
     * order of the data being read, if applicable.
     *
     * If the reader is constructed as binary_reader<tdsl::endian::big>,
     * that means the data is assumed to be in big endian byte order
     * (e.g. most network protocols). When read function is called in a
     * little-endian environment, the data will be first read, then byte
     * order will be swapped before returning result to the caller.
     *
     * The default read() byte order is the reader's byte order. The user
     * can override this by supplying explicit ReadEndianness template
     * parameter.
     *
     * @tparam ReaderEndianness Default endianness of the reader
     */
    template <tdsl::endian ReaderEndianness>
    class binary_reader : private tdsl::span<const tdsl::uint8_t> {
    public:
        using span_type = tdsl::span<const tdsl::uint8_t>;
        // Expose span constructors
        using span_type::span;
        using span_type::operator bool;
        using span_type::data;
        using span_type::size_bytes;

        /**
         * Copy constructor
         */
        explicit constexpr binary_reader(const span_type & other) : span_type(other) {}

        /**
         * Move constructor
         */
        explicit constexpr binary_reader(span_type && other) : span_type(TDSL_MOVE(other)) {}

        /**
         * Read a value with type T from current reader position.
         * The read value will be in host's byte order.
         *
         * @tparam T Type to read
         * @tparam ReadEndianness Endianness of the data (default=ReaderEndianness)
         *
         * @returns sizeof(T) bytes read from current position as-is if ReadEndianness==HostEndianness
         * @returns sizeof(T) bytes read from current position (byte-order swapped) if ReadEndianness!=HostEndianness
         */
        template <typename T, tdsl::endian ReadEndianness = ReaderEndianness>
        inline TDSL_NODISCARD TDSL_CXX14_CONSTEXPR auto read() noexcept -> T {
            return tdsl::swap_endianness<ReaderEndianness, tdsl::endian::native>(read_raw<T>());
        }

        /**
         * Make a subreader with @p size at current position.
         *
         * @param size Size of the subreader. size + current + offset be in bounds of current reader. If not,
         *             size will be automatically truncated to remaining_bytes
         *
         * @return A new reader in [current - next(min(size, remaining_bytes))] range
         */
        template <tdsl::endian SubreaderEndianness = ReaderEndianness>
        inline auto subreader(tdsl::uint32_t size) const noexcept -> binary_reader<SubreaderEndianness> {

            if (!has_bytes(size)) {
                return binary_reader<SubreaderEndianness>{nullptr, static_cast<tdsl::uint32_t>(0)};
            }
            return binary_reader<SubreaderEndianness>(current(), size);
        }

        /**
         * Read `number_of_elements` elements from current reader position and progress
         * reader position by size of read.
         *
         * @param number_of_elements How many elements
         * @return std::span<const T>
         */
        TDSL_NODISCARD inline TDSL_CXX14_CONSTEXPR auto read(tdsl::uint32_t number_of_elements) noexcept -> span_type {

            if (number_of_elements > 0 && has_bytes(number_of_elements)) {
                span_type result{current(), (current() + number_of_elements)};
                do_advance(number_of_elements);
                return result;
            }
            return span_type(/*elem=*/nullptr, static_cast<tdsl::uint32_t>(0));
        }

        /**
         * Read a value with type T from current reader position,
         * without converting its' endianness.
         *
         * @tparam T Type to read
         * @return A value with sizeof(T) bytes, read as type T
         */
        template <typename T>
        inline TDSL_CXX14_CONSTEXPR auto read_raw() noexcept -> T {
            if (not has_bytes(sizeof(T))) {
                TDSL_ASSERT(false);
                TDSL_UNREACHABLE;
            }
            T result{};
            // This is for complying the strict aliasing rules
            // for type T. The compiler should optimize this
            // call away.
            memcpy(&result, current(), sizeof(T));
            do_advance(sizeof(T));
            return result;
        }

        /**
         * Get pointer to the current read position
         *
         * @return const tdsl::uint8_t* Pointer to the current read position
         */
        inline TDSL_CXX14_CONSTEXPR auto current() const noexcept -> const tdsl::uint8_t * {
            TDSL_ASSERT(offset_ <= size_bytes());
            return data() + offset_;
        }

        /**
         * Current read offset
         * (a.k.a. amount of bytes read until now)
         *
         * @return tdsl::uint32_t Current read offset
         */
        inline TDSL_CXX14_CONSTEXPR auto offset() const noexcept -> tdsl::uint32_t {
            TDSL_ASSERT(offset_ <= size_bytes());
            return offset_;
        }

        /**
         * Set reader's offset to a specific position
         *
         * @param [in] pos New offset value
         * @return true @p pos is in binary_reader's bounds and @ref offset_ is set to @p pos
         * @return false otherwise
         *
         * @note @ref offset_ is guaranteed to be not modified when the
         * result is false
         */
        inline TDSL_CXX14_CONSTEXPR bool seek(tdsl::uint32_t pos) noexcept {

            // Check boundaries
            if (pos >= size_bytes()) {
                return false;
            }
            offset_ = pos;
            TDSL_ASSERT(offset_ <= size_bytes());
            return true;
        }

        /**
         * Reset read offset to zero
         */
        inline TDSL_CXX14_CONSTEXPR void reset() noexcept {
            offset_ = {0};
        }

        /**
         * Advance current reader offset by @p amount_of_bytes bytes
         *
         * @param [in] amount_of_bytes Amount to advance (can be negative)
         * @returns true if @p amount_of_bytes + current offset is in binary_reader's bounds
         *          and offset is advanced by @p amount_of_bytes bytes
         * @returns false otherwise
         *
         * @note @ref offset_ is guaranteed to be not modified when the
         * result is false
         */
        inline TDSL_CXX14_CONSTEXPR bool advance(tdsl::int32_t amount_of_bytes) noexcept {
            // Check boundaries
            if ((static_cast<tdsl::int64_t>(offset_) + static_cast<tdsl::int64_t>(amount_of_bytes)) > size_bytes() ||
                (static_cast<tdsl::int64_t>(offset_) + amount_of_bytes) < 0) {
                return false;
            }
            do_advance(amount_of_bytes);
            TDSL_ASSERT(offset_ <= size_bytes());
            return true;
        }

        /**
         * Check whether reader has @p amount_of_bytes bytes remaining
         *
         * @param [in] amount_of_bytes Amount of bytes for check
         * @return true if reader has at least v bytes
         * @return false otherwise
         */
        inline TDSL_CXX14_CONSTEXPR bool has_bytes(tdsl::uint32_t amount_of_bytes) const noexcept {
            // Promote offset and v to next greater signed integer type
            // since the result of the sum may overflow
            return (tdsl::int64_t{offset_} + tdsl::int64_t{amount_of_bytes}) <= tdsl::int64_t{size_bytes()};
        }

        /**
         * @brief Remaining bytes to read, relative to @ref `size`
         *
         * @return std::int64_t Amount of bytes remaining
         */
        TDSL_NODISCARD
        inline constexpr auto remaining_bytes() const noexcept -> tdsl::int64_t {
            return tdsl::int64_t{size_bytes()} - tdsl::int64_t{offset()};
        }

    private:
        /**
         * Advance @ref offset_ by @p amount_of_bytes bytes without any boundary checking.
         *
         * @param [in] amount_of_bytes Amount to advance
         */
        inline TDSL_CXX14_CONSTEXPR void do_advance(tdsl::int32_t amount_of_bytes) noexcept {

            TDSL_ASSERT(((tdsl::int64_t{offset()} + amount_of_bytes) >= 0) &&
                        ((tdsl::int64_t{offset()} + amount_of_bytes) <= size_bytes()));
            offset_ += amount_of_bytes;
        }

        tdsl::uint32_t offset_{}; /**<! Current read offset */

    }; // class binary_reader <>

} // namespace tdsl

/**
 * shortcut macro to return N if READER does
 * not have at least N bytes
 */
#define TDSL_RETIF_LESS_BYTES(READER, N)                                                                                                   \
    do {                                                                                                                                   \
        if (not READER.has_bytes(N)) {                                                                                                     \
            return N - READER.remaining_bytes();                                                                                           \
        }                                                                                                                                  \
    } while (0)

#endif