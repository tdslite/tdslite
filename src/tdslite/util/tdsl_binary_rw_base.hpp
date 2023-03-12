/**
 * ____________________________________________________
 * Base class implementation for binary_<reader/writer>
 *
 * @file   tdsl_binary_rw_base.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   18.11.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */
#ifndef TDSL_UTIL_TDSL_BINARY_RW_BASE_HPP
#define TDSL_UTIL_TDSL_BINARY_RW_BASE_HPP

#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_type_traits.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_debug_print.hpp>

namespace tdsl {

    /**
     * Binary R/W base class
     *
     * @tparam Derived Deriving binary R/W type
     */
    template <typename Derived>
    struct binary_reader_writer_base {

        // --------------------------------------------------------------------------------

        /**
         * The checkpoint object
         *
         * Allows user to return to a specific point in
         * binary reader/writer back.
         */
        struct checkpoint {

            /**
             * Construct a new checkpoint object
             *
             * @param [in] r Binary reader/writer base object
             */
            inline checkpoint(binary_reader_writer_base<Derived> & r) :
                rw_base(r), offset(rw_base.offset()) {}

            /**
             * Revert binary reader/writer offset back
             * to the saved checkpoint. Note that this
             * function does not revert any written data
             * for binary writers, so be careful for sensitive
             * data.
             */
            inline void restore() noexcept {
                rw_base.seek(offset);
            }

        private:
            binary_reader_writer_base<Derived> & rw_base;
            tdsl::size_t offset;
        };

        // --------------------------------------------------------------------------------

        /**
         * Get pointer to the current position
         *
         * @return const tdsl::uint8_t* Pointer to the current position
         */
        template <typename U = Derived>
        inline TDSL_NODISCARD auto current() const noexcept
            -> decltype(traits::declval<U>().data()) {
            TDSL_ASSERT(offset_ <= static_cast<const U &>(*this).size_bytes());
            return static_cast<const U &>(*this).data() + offset_;
        }

        // --------------------------------------------------------------------------------

        /**
         * @brief Remaining capacity (in bytes), relative to @ref `size`
         *
         * @return std::int64_t Amount of bytes remaining
         */
        inline TDSL_NODISCARD auto remaining_bytes() const noexcept -> tdsl::size_t {
            return static_cast<const Derived &>(*this).size_bytes() - offset();
        }

        // --------------------------------------------------------------------------------

        /**
         * Current offset
         */
        inline TDSL_NODISCARD auto offset() const noexcept -> tdsl::size_t {
            TDSL_ASSERT(offset_ <= static_cast<const Derived &>(*this).size_bytes());
            return offset_;
        }

        // --------------------------------------------------------------------------------

        /**
         * Save current reader/writer position into
         * a checkpoint object. Invoke restore() function to
         * return to the saved position back.
         *
         * It is useful for scenarios such as the user needs to keep the
         * data in the buffer when e.g. the data is incomplete or needed
         * to be persisted for an extended duration of time.
         *
         * @return checkpoint Checkpoint object at current offset
         */
        inline TDSL_NODISCARD auto checkpoint() noexcept -> checkpoint {
            return {*this};
        }

        // --------------------------------------------------------------------------------

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
        inline auto seek(tdsl::size_t pos) noexcept -> bool {
            // Check boundaries
            if (pos >= static_cast<Derived &>(*this).size_bytes()) {
                return false;
            }
            offset_ = pos;
            TDSL_ASSERT(offset_ <= static_cast<Derived &>(*this).size_bytes());
            return true;
        }

        // --------------------------------------------------------------------------------

        /**
         * Reset offset to zero
         */
        inline auto reset() noexcept -> void {
            offset_ = {0};
        }

        // --------------------------------------------------------------------------------

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
        inline auto advance(tdsl::ssize_t amount_of_bytes) noexcept -> bool {

            // If moving forwards, ensure we got that much amount
            if (amount_of_bytes >= 0) {
                if ((static_cast<tdsl::size_t>(amount_of_bytes) > remaining_bytes())) {
                    return false;
                }
            }
            else {
                // If moving backwards, the offset must be at
                // at least `amount of bytes`
                if (static_cast<tdsl::size_t>(-(amount_of_bytes)) > offset()) {
                    return false;
                }
            }

            do_advance(amount_of_bytes);
            TDSL_ASSERT(offset_ <= static_cast<Derived &>(*this).size_bytes());
            return true;
        }

        // --------------------------------------------------------------------------------

        /**
         * Check whether Derived has @p amount_of_bytes empty space remaining
         *
         * @param [in] amount_of_bytes Amount of bytes for check
         * @return true if Derived has at least v bytes
         * @return false otherwise
         */
        inline TDSL_NODISCARD auto has_bytes(tdsl::size_t amount_of_bytes) const noexcept -> bool {

            // overflow check
            if (amount_of_bytes > (tdsl::numeric_limits::max_value<tdsl::size_t>() - offset())) {
                return false;
            }

            return (offset() + amount_of_bytes) <= static_cast<const Derived &>(*this).size_bytes();
        }

        // --------------------------------------------------------------------------------

        /**
         * Check whether Derived has @p amount_of_bytes empty space remaining
         *
         * @param [in] amount_of_bytes Amount of bytes for check
         * @param [in] offset Override value for offset
         * @return true if Derived has at least v bytes
         * @return false otherwise
         */
        inline TDSL_NODISCARD auto has_bytes(tdsl::size_t amount_of_bytes,
                                             tdsl::size_t offset) const noexcept -> bool {

            // overflow check
            if (amount_of_bytes > (tdsl::numeric_limits::max_value<tdsl::size_t>() - offset)) {
                return false;
            }

            // offset + amount_of_bytes guaranteed to be <=
            // tdsl::numeric_limits<tdsl::size_t>::max()
            return (offset + amount_of_bytes) <= static_cast<const Derived &>(*this).size_bytes();
        }

        /**
         * Beginning of the occupied (non-free) region
         */
        inline TDSL_NODISCARD auto inuse_begin() const noexcept -> const tdsl::uint8_t * {
            return static_cast<const Derived &>(*this).begin();
        }

        // --------------------------------------------------------------------------------

        /**
         * End of the occupied (non-free) region
         */
        inline TDSL_NODISCARD auto inuse_end() const noexcept -> const tdsl::uint8_t * {
            TDSL_ASSERT((inuse_begin() + this->offset_) <=
                        static_cast<const Derived &>(*this).end());
            return inuse_begin() + this->offset();
        }

        // --------------------------------------------------------------------------------

        /**
         * View to written data
         *
         * @return tdsl::byte_view
         */
        inline TDSL_NODISCARD auto inuse_span() const noexcept -> tdsl::byte_view {
            return tdsl::byte_view{inuse_begin(), inuse_end()};
        }

        // --------------------------------------------------------------------------------

        /**
         * Beginning of the free space
         *
         * @return tdsl::uint8_t*
         */
        inline TDSL_NODISCARD auto free_begin() const noexcept -> tdsl::uint8_t * {
            return static_cast<const Derived &>(*this).begin() + this->offset();
        }

        // --------------------------------------------------------------------------------

        /**
         * End of the free space
         *
         * @return tdsl::uint8_t*
         */
        inline TDSL_NODISCARD auto free_end() const noexcept -> tdsl::uint8_t * {
            return static_cast<const Derived &>(*this).end();
        }

        // --------------------------------------------------------------------------------

        /**
         * Span to free space
         *
         * @return tdsl::byte_span
         */
        inline TDSL_NODISCARD auto free_span() const noexcept -> tdsl::byte_span {
            return tdsl::byte_span{free_begin(), free_end()};
        }

    protected:
        /**
         * Advance @ref offset_ by @p amount_of_bytes bytes without any boundary checking.
         *
         * @param [in] amount_of_bytes Amount to advance
         */
        inline void do_advance(tdsl::ssize_t amount_of_bytes) noexcept {
            if (amount_of_bytes >= 0) {
                if ((offset() + static_cast<tdsl::size_t>(amount_of_bytes)) >
                    static_cast<Derived &>(*this).size_bytes()) {
                    TDSL_ASSERT_MSG(
                        false,
                        "Offset is going to be OOB when advance(amount_of_bytes) is applied!");
                    TDSL_TRAP;
                }
            }
            else {
                if (static_cast<tdsl::size_t>(-amount_of_bytes) > offset()) {
                    TDSL_ASSERT_MSG(
                        false,
                        "Offset is going to be OOB when advance(amount_of_bytes) is applied!");
                    TDSL_TRAP;
                }
            }

            offset_ += amount_of_bytes;
        }

        tdsl::size_t offset_{}; /**<! Current offset */
    };
} // namespace tdsl

#endif