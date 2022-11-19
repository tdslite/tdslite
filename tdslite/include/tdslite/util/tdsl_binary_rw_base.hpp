/**
 * _________________________________________________
 *
 * @file   tdsl_progressive_cursor_mixin.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   18.11.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */
#ifndef TDSL_UTIL_TDS_PROGRESSIVE_CURSOR_MIXIN_HPP
#define TDSL_UTIL_TDS_PROGRESSIVE_CURSOR_MIXIN_HPP

#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_span.hpp>

namespace tdsl {
    template <typename Derived>
    struct binary_reader_writer_base {

        /**
         * Get pointer to the current position
         *
         * @return const tdsl::uint8_t* Pointer to the current position
         */
        template <typename U = Derived>
        inline TDSL_NODISCARD TDSL_CXX14_CONSTEXPR auto current() const noexcept
            -> decltype(traits::declval<U>().data()) {
            TDSL_ASSERT(offset_ <= static_cast<const U &>(*this).size_bytes());
            return static_cast<const U &>(*this).data() + offset_;
        }

        /**
         * @brief Remaining capacity (in bytes), relative to @ref `size`
         *
         * @return std::int64_t Amount of bytes remaining
         */

        inline TDSL_NODISCARD constexpr auto remaining_bytes() const noexcept -> tdsl::int64_t {
            return tdsl::int64_t{static_cast<const Derived &>(*this).size_bytes()} -
                   tdsl::int64_t{offset()};
        }

        /**
         * Current offset
         *
         * @return tdsl::uint32_t Current  offset
         */
        inline TDSL_NODISCARD TDSL_CXX14_CONSTEXPR auto offset() const noexcept -> tdsl::uint32_t {
            TDSL_ASSERT(offset_ <= static_cast<const Derived &>(*this).size_bytes());
            return offset_;
        }

        /**
         * Set offset to a specific position
         *
         * @param [in] pos New offset value
         * @return true @p pos is in Derived's bounds and @ref offset_ is set to @p pos
         * @return false otherwise
         *
         * @note @ref offset_ is guaranteed to be not modified when the
         * result is false
         */
        inline TDSL_CXX14_CONSTEXPR auto seek(tdsl::uint32_t pos) noexcept -> bool {
            // Check boundaries
            if (pos >= static_cast<Derived &>(*this).size_bytes()) {
                return false;
            }
            offset_ = pos;
            TDSL_ASSERT(offset_ <= static_cast<Derived &>(*this).size_bytes());
            return true;
        }

        /**
         * Reset offset to zero
         */
        inline TDSL_CXX14_CONSTEXPR auto reset() noexcept -> void {
            offset_ = {0};
        }

        /**
         * Advance offset by @p amount_of_bytes bytes
         *
         * @param [in] amount_of_bytes Amount to advance (can be negative)
         * @returns true if @p amount_of_bytes + current offset is in Derivedr's bounds
         *          and offset is advanced by @p amount_of_bytes bytes
         * @returns false otherwise
         *
         * @note @ref offset_ is guaranteed to be not modified when the
         * result is false
         */
        inline TDSL_CXX14_CONSTEXPR auto advance(tdsl::int32_t amount_of_bytes) noexcept -> bool {
            // Check boundaries
            if ((static_cast<tdsl::int64_t>(offset_) +
                 static_cast<tdsl::int64_t>(amount_of_bytes)) >
                    static_cast<Derived &>(*this).size_bytes() ||
                (static_cast<tdsl::int64_t>(offset_) + amount_of_bytes) < 0) {
                return false;
            }
            do_advance(amount_of_bytes);
            TDSL_ASSERT(offset_ <= static_cast<Derived &>(*this).size_bytes());
            return true;
        }

        /**
         * Check whether Derived has @p amount_of_bytes empty space remaining
         *
         * @param [in] amount_of_bytes Amount of bytes for check
         * @return true if Derived has at least v bytes
         * @return false otherwise
         */
        inline TDSL_NODISCARD TDSL_CXX14_CONSTEXPR auto
        has_bytes(tdsl::uint32_t amount_of_bytes) const noexcept -> bool {
            // Promote offset and v to next greater signed integer type
            // since the result of the sum may overflow
            return (tdsl::int64_t{offset_} + tdsl::int64_t{amount_of_bytes}) <=
                   tdsl::int64_t{static_cast<const Derived &>(*this).size_bytes()};
        }

        /**
         * Check whether Derived has @p amount_of_bytes empty space remaining
         *
         * @param [in] amount_of_bytes Amount of bytes for check
         * @param [in] offset Override value for offset
         * @return true if Derived has at least v bytes
         * @return false otherwise
         */
        inline TDSL_NODISCARD TDSL_CXX14_CONSTEXPR auto
        has_bytes(tdsl::uint32_t amount_of_bytes, tdsl::uint32_t offset) const noexcept -> bool {
            // Promote offset and v to next greater signed integer type
            // since the result of the sum may overflow
            return (tdsl::int64_t{offset} + tdsl::int64_t{amount_of_bytes}) <=
                   tdsl::int64_t{static_cast<const Derived &>(*this).size_bytes()};
        }

        /**
         * Beginning of the occupied (non-free) region
         */
        inline auto inuse_begin() const noexcept -> const tdsl::uint8_t * {
            return static_cast<const Derived &>(*this).begin();
        }

        /**
         * End of the occupied (non-free) region
         */
        inline auto inuse_end() const noexcept -> const tdsl::uint8_t * {
            TDSL_ASSERT((inuse_begin() + this->offset_) <=
                        static_cast<const Derived &>(*this).end());
            return inuse_begin() + this->offset();
        }

        /**
         * View to written data
         *
         * @return tdsl::byte_view
         */
        inline auto inuse_span() const noexcept -> tdsl::byte_view {
            return tdsl::byte_view{inuse_begin(), inuse_end()};
        }

        /**
         * Beginning of the free space
         *
         * @return tdsl::uint8_t*
         */
        inline auto free_begin() const noexcept -> tdsl::uint8_t * {
            return static_cast<const Derived &>(*this).begin() + this->offset();
        }

        /**
         * End of the free space
         *
         * @return tdsl::uint8_t*
         */
        inline auto free_end() const noexcept -> tdsl::uint8_t * {
            return static_cast<const Derived &>(*this).end();
        }

        /**
         * Span to free space
         *
         * @return tdsl::byte_span
         */
        inline auto free_span() const noexcept -> tdsl::byte_span {
            return tdsl::byte_span{free_begin(), free_end()};
        }

    protected:
        /**
         * Advance @ref offset_ by @p amount_of_bytes bytes without any boundary checking.
         *
         * @param [in] amount_of_bytes Amount to advance
         */
        inline TDSL_CXX14_CONSTEXPR void do_advance(tdsl::int32_t amount_of_bytes) noexcept {

            TDSL_ASSERT(((tdsl::int64_t{offset()} + amount_of_bytes) >= 0) &&
                        ((tdsl::int64_t{offset()} + amount_of_bytes) <=
                         static_cast<Derived &>(*this).size_bytes()));
            offset_ += amount_of_bytes;
        }

        tdsl::uint32_t offset_{}; /**<! Current offset */
    };
} // namespace tdsl

#endif