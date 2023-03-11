/**
 * ____________________________________________________
 * binary_reader<> utility class implementation
 *
 * @file   tdsl_binary_reader.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_UTIL_TDS_BINARY_READER_HPP
#define TDSL_UTIL_TDS_BINARY_READER_HPP

#include <tdslite/util/tdsl_byte_swap.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_endian.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_binary_rw_base.hpp>

#include <string.h> // needed for memcpy

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
     * @tparam DataEndianness Endianness of the data being read
     */
    template <tdsl::endian DataEndianness>
    class binary_reader : private byte_view,
                          public binary_reader_writer_base<binary_reader<DataEndianness>> {
    public:
        using span_type = byte_view;
        // Expose span constructors
        using span_type::span;
        using span_type::operator bool;
        using span_type::begin;
        using span_type::data;
        using span_type::end;
        using span_type::size_bytes;

        // --------------------------------------------------------------------------------

        /**
         * Copy constructor
         */
        explicit constexpr binary_reader(const span_type & other) noexcept : span_type(other) {}

        // --------------------------------------------------------------------------------

        /**
         * Move constructor
         */
        explicit constexpr binary_reader(span_type && other) noexcept :
            span_type(TDSL_MOVE(other)) {}

        // --------------------------------------------------------------------------------

        /**
         * Read a value with type T from current reader position.
         * The read value will be in host's byte order.
         *
         * @tparam T Type to read
         * @tparam ReadEndianness Endianness of the data (default=ReaderEndianness)
         *
         * @returns sizeof(T) bytes read from current position as-is if
         * ReadEndianness==HostEndianness
         * @returns sizeof(T) bytes read from current position (byte-order swapped) if
         * ReadEndianness!=HostEndianness
         */
        template <typename T, tdsl::endian ReadEndianness = DataEndianness>
        inline TDSL_NODISCARD auto read() noexcept -> T {
            return tdsl::swap_endianness<ReadEndianness, tdsl::endian::native>(read_raw<T>());
        }

        // --------------------------------------------------------------------------------

        /**
         * Make a subreader with @p size at current position.
         *
         * @param size Size of the subreader. size + current + offset be in bounds of current
         * reader. If not, size will be automatically truncated to remaining_bytes
         *
         * @return A new reader in [current - next(min(size, remaining_bytes))] range
         */
        template <tdsl::endian SubreaderEndianness = DataEndianness>
        inline TDSL_NODISCARD auto subreader(tdsl::size_t size) const noexcept
            -> binary_reader<SubreaderEndianness> {
            if (!this->has_bytes(size)) {
                return binary_reader<SubreaderEndianness>{nullptr, static_cast<tdsl::size_t>(0)};
            }
            return binary_reader<SubreaderEndianness>(this->current(), size);
        }

        // --------------------------------------------------------------------------------

        /**
         * Read `number_of_elements` elements from current reader position and progress
         * reader position by size of read.
         *
         * @param number_of_elements How many elements
         * @return std::span<const T>
         */
        inline TDSL_NODISCARD auto read(tdsl::size_t number_of_elements) noexcept -> span_type {
            if (number_of_elements > 0 && this->has_bytes(number_of_elements)) {
                span_type result{this->current(), (this->current() + number_of_elements)};
                this->do_advance(number_of_elements);
                return result;
            }
            return span_type(/*begin=*/nullptr, /*end=*/nullptr);
        }

        // --------------------------------------------------------------------------------

        /**
         * Read a value with type T from current reader position,
         * without converting its' endianness and then advance the
         * reader position by sizeof(T).
         *
         * @tparam T Type to read
         * @return A value with sizeof(T) bytes, read as type T
         */
        template <typename T, typename MemcpyFnT = decltype(::memcpy)>
        inline TDSL_NODISCARD auto read_raw(MemcpyFnT f = ::memcpy) noexcept -> T {
            auto result = peek_raw<T, MemcpyFnT>(f);
            this->do_advance(sizeof(T));
            return result;
        }

        // --------------------------------------------------------------------------------

        /**
         * Read a value with type T from current reader position,
         * without converting its' endianness.
         *
         * @tparam T Type to read
         * @return A value with sizeof(T) bytes, read as type T
         */
        template <typename T, typename MemcpyFnT = decltype(::memcpy)>
        inline TDSL_NODISCARD auto peek_raw(MemcpyFnT f = ::memcpy) const noexcept -> T {
            if (not this->has_bytes(sizeof(T))) {
                TDSL_ASSERT_MSG(false, "Unchecked read, check size before reading!");
                TDSL_TRAP;
            }
            T result{};
            // This is for complying the strict aliasing rules
            // for type T. The compiler should optimize this
            // call away.
            f(&result, this->current(), sizeof(T));
            return result;
        }

    }; // class binary_reader <>

} // namespace tdsl

// --------------------------------------------------------------------------------

/**
 * shortcut macro to return N if READER does
 * not have at least N bytes
 */
#define TDSL_RETIF_LESS_BYTES(READER, N)                                                           \
    do {                                                                                           \
        if (not READER.has_bytes(N)) {                                                             \
            return N - READER.remaining_bytes();                                                   \
        }                                                                                          \
    } while (0)

#endif

// --------------------------------------------------------------------------------

#define TDSL_TRY_READ_VARCHAR(TYPE, VARNAME, READER)                                               \
    const auto VARNAME##_octets = (READER.read<TYPE>() * 2);                                       \
    if (not READER.has_bytes(VARNAME##_octets)) {                                                  \
        return VARNAME##_octets - READER.remaining_bytes();                                        \
    }                                                                                              \
    const auto VARNAME = READER.read(VARNAME##_octets)

// --------------------------------------------------------------------------------

#define TDSL_TRY_READ_U16_VARCHAR(VARNAME, READER)                                                 \
    TDSL_TRY_READ_VARCHAR(tdsl::uint16_t, VARNAME, READER)

// --------------------------------------------------------------------------------

#define TDSL_TRY_READ_U8_VARCHAR(VARNAME, READER)                                                  \
    TDSL_TRY_READ_VARCHAR(tdsl::uint8_t, VARNAME, READER)
