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

#ifndef TDSLITE_DETAIL_TDS_BINARY_READER_HPP
#define TDSLITE_DETAIL_TDS_BINARY_READER_HPP

#include <tdslite/detail/tds_byte_swap.hpp>
#include <tdslite/detail/tds_inttypes.hpp>
#include <tdslite/detail/tds_endian.hpp>
#include <tdslite/detail/tds_span.hpp>
#include <tdslite/detail/tds_type_traits.hpp>
#include <tdslite/detail/tds_macrodef.hpp>

namespace tdslite {

    /**
     * A helper class to read from contiguous stream of bytes
     *
     * Reader is endianness-aware, meaning that if host endianness
     * and data endianness are different, reader will swap the byte
     * order of the data being read, if applicable.
     *
     * If the reader is constructed as binary_reader<tdslite::endian::big>,
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
    template <tdslite::endian ReaderEndianness>
    class binary_reader : private tdslite::span {
    public:
        // Expose span constructors
        using tdslite::span::span;

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
        template <typename T, tdslite::endian ReadEndianness = ReaderEndianness>
        inline TDSLITE_CXX14_CONSTEXPR auto read() noexcept -> T {
            return tdslite::detail::swap_endianness<ReaderEndianness, tdslite::endian::native>(read_raw<T>());
        }

        /**
         * Read a value with type T from current reader position,
         * without converting its' endianness.
         *
         * @tparam T Type to read
         * @return A value with sizeof(T) bytes, read as type T
         */
        template <typename T>
        inline TDSLITE_CXX14_CONSTEXPR auto read_raw() noexcept -> T {
            if (not has_bytes(sizeof(T))) {
                TDSLITE_ASSERT(false);
                TDSLITE_UNREACHABLE;
            }
            T result{};
            // This is for complying the strict aliasing rules
            // for type T. The compiler (usually) optimizes this
            // call away.
            memcpy(&result, current(), sizeof(T));
            do_advance(sizeof(T));
            return result;
        }

        /**
         * Get pointer to the current read position
         *
         * @return const tdslite::uint8_t* Pointer to the current read position
         */
        inline TDSLITE_CXX14_CONSTEXPR auto current() const noexcept -> const tdslite::uint8_t * {
            TDSLITE_ASSERT(data + offset_ < data + size);
            return data + offset_;
        }

        /**
         * Current read offset
         * (a.k.a. amount of bytes read until now)
         *
         * @return tdslite::uint32_t Current read offset
         */
        inline TDSLITE_CXX14_CONSTEXPR auto offset() const noexcept -> tdslite::uint32_t {
            TDSLITE_ASSERT(offset_ < size);
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
        inline TDSLITE_CXX14_CONSTEXPR bool seek_to(tdslite::uint32_t pos) noexcept {

            // Check boundaries
            if (pos >= size) {
                return false;
            }
            offset_ = pos;
            TDSLITE_ASSERT(offset_ < size);
            return true;
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
        inline TDSLITE_CXX14_CONSTEXPR bool advance(tdslite::int32_t amount_of_bytes) noexcept {
            // Check boundaries
            if ((offset_ + amount_of_bytes) >= size || (offset_ - amount_of_bytes) < 0) {
                return false;
            }
            do_advance(amount_of_bytes);
            TDSLITE_ASSERT(offset_ < size);
            return true;
        }

        /**
         * Check whether reader has @p amount_of_bytes bytes remaining
         *
         * @param [in] amount_of_bytes Amount of bytes for check
         * @return true if reader has at least v bytes
         * @return false otherwise
         */
        inline TDSLITE_CXX14_CONSTEXPR bool has_bytes(tdslite::uint32_t amount_of_bytes) const noexcept {
            // Promote offset and v to next greater signed integer type
            // since the result of the sum may overflow
            return (tdslite::int64_t{offset_} + tdslite::int64_t{amount_of_bytes}) <= tdslite::int64_t{size};
        }

    private:
        /**
         * Advance @ref offset_ by @p amount_of_bytes bytes without any boundary checking.
         *
         * @param [in] amount_of_bytes Amount to advance
         */
        inline TDSLITE_CXX14_CONSTEXPR void do_advance(tdslite::int32_t amount_of_bytes) noexcept {
            TDSLITE_ASSERT(offset + amount_of_bytes >= 0 && offset + amount_of_bytes < size);
            offset_ += amount_of_bytes;
        }

        tdslite::uint32_t offset_{}; /**<! Current read offset */

    }; // class binary_reader <>

} // namespace tdslite

#endif