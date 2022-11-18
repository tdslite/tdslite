/**
 * ____________________________________________________
 * binary_writer<> utility class implementation
 *
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   18/11/2022
 *
 * SPDX-License-Identifier: MIT
 * ____________________________________________________
 */

#ifndef TDSL_UTIL_TDS_BINARY_WRITER_HPP
#define TDSL_UTIL_TDS_BINARY_WRITER_HPP

#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_endian.hpp>
#include <tdslite/util/tdsl_byte_swap.hpp>
#include <tdslite/util/tdsl_binary_rw_base.hpp>

#include <string.h>

namespace tdsl {

    /**
     * A helper class to write contiguous stream of bytes
     *
     * Writer is endianness-aware, meaning that if host endianness
     * and data endianness are different, writer will swap the byte
     * order of the data being written, if applicable.
     *
     * If the writter is constructed as binary_writer<DataEndianness>,
     * that means the data being written will be written in DataEndianness
     * byte order, regardless of the host byte order.
     *
     * The default write() byte order is the writer's byte order. The user
     * can override this by supplying explicit WriteEndianness template
     * parameter.
     *
     * @tparam DataEndianness Endianness of the data written
     */
    template <tdsl::endian DataEndianness>
    struct binary_writer : private byte_span,
                           public binary_reader_writer_base<binary_writer<DataEndianness>> {

        using span_type = byte_span;
        // Expose span constructors
        using span_type::span_type;
        using span_type::operator bool;
        using span_type::data;
        using span_type::size_bytes;

        /**
         * Copy constructor
         */
        explicit constexpr binary_writer(const span_type & other) noexcept : span_type(other) {}

        /**
         * Move constructor
         */
        explicit constexpr binary_writer(span_type && other) noexcept :
            span_type(TDSL_MOVE(other)) {}

        /**
         * Write @p data as-is
         *
         * @param [in] data Data to write
         * @return true if writer has enough space for @p data and bytes are
         * written
         * @return false if writer does not have enough space. The underlying
         * span will be unmodified in this case.
         */
        inline TDSL_NODISCARD TDSL_CXX14_CONSTEXPR auto write(tdsl::byte_view data) noexcept
            -> bool {
            if (not this->has_bytes(data.size_bytes())) {
                return false;
            }

            memcpy(this->current(), data.data(), data.size_bytes());
            this->do_advance(data.size_bytes());
            return true;
        };

        /**
         * Write a value with type T to current position.
         * The write value will be in WriteEndianness byte order.
         *
         * @tparam T Type of @p value (auto-deduced)
         * @tparam WriteEndianness Endianness of the data (default=DataEndianness)
         *
         * @param [in] value Value to write
         *
         * @returns true if writer has enough space for sizeof(T),
         * bytes of T are written
         * @returns false if writer does not have enough space. The underlying span
         * is unmodified in this case.
         */
        template <tdsl::endian WriteEndianness = DataEndianness, typename T>
        inline TDSL_NODISCARD TDSL_CXX14_CONSTEXPR auto write(T value) noexcept -> bool {
            if (not this->has_bytes(sizeof(T))) {
                return false;
            }

            const T result{tdsl::swap_endianness<tdsl::endian::native, WriteEndianness>(value)};
            // This is for complying the strict aliasing rules
            // for type T. The compiler should optimize this
            // call away.
            memcpy(this->current(), &result, sizeof(T));
            this->do_advance(sizeof(T));
            return true;
        }

        /**
         * Write a value with type T to current position
         * as-is (no endianness conversion).
         *
         * @tparam T type of @p value (auto-deduced)
         * @param [in] value Value to write
         *
         * @returns true if writer has enough space for sizeof(T),
         * bytes of T are written
         * @returns false if writer does not have enough space. The underlying span
         * is unmodified in this case.
         */
        template <typename T>
        inline TDSL_NODISCARD TDSL_CXX14_CONSTEXPR auto write_raw(T value) noexcept -> bool {
            if (not this->has_bytes(sizeof(T))) {
                return false;
            }
            // This is for complying the strict aliasing rules
            // for type T. The compiler should optimize this
            // call away.
            memcpy(&value, this->current(), sizeof(T));
            this->do_advance(sizeof(T));
            return true;
        }

        /**
         * Beginning of the written data
         */
        inline auto dw_begin() const noexcept -> const tdsl::uint8_t * {
            return begin();
        }

        /**
         * End of the written data
         */
        inline auto dw_end() const noexcept -> const tdsl::uint8_t * {
            TDSL_ASSERT((dw_begin() + this->offset_) <= end());
            return dw_begin() + this->offset();
        }
    };

} // namespace tdsl

#endif