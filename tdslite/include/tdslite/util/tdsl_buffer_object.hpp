/**
 * ____________________________________________________
 * tdsl_buffer_object implementation
 *
 * @file   tdsl_buffer_object.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   19.11.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_UTIL_TDSL_BUFFER_OBJECT_HPP
#define TDSL_UTIL_TDSL_BUFFER_OBJECT_HPP

#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_binary_writer.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <tdslite/util/tdsl_debug_print.hpp>

namespace tdsl {

    /**
     * A buffer object that allows exclusive progressive reads
     * and writes over underlying buffer span.
     *
     * *Progressive* meaning the underlying buffer will be modified when
     * data is read and written. Writes are immediate, but read operations
     * are committed to underlying buffer (i.e. discarded) when progressive
     * _binary_reader object is destructed.
     *
     * The underlying buffer can be read through progressive_binary_reader
     * obtained via .get_reader() function call and can be modified through
     * progressive_binary_writer obtained via .get_writer().
     */
    struct tdsl_buffer_object : private tdsl::binary_writer<tdsl::endian::little> {
        using binary_writer_type = tdsl::binary_writer<tdsl::endian::little>;
        using binary_reader_type = tdsl::binary_reader<tdsl::endian::little>;
        using binary_writer_type::binary_writer;

        /**
         * The amount read from this reader object will be discarded
         * from the underlying buffer and will be made available back.
         */
        struct progressive_binary_reader {

            inline progressive_binary_reader(binary_writer_type & w, bool & mef) :
                writer{w}, reader{w.data(), w.offset()}, in_use_flag(mef) {

                if (in_use_flag) {
                    TDSL_ASSERT_MSG(false, "Buffer object is already in use!");
                    TDSL_TRAP;
                }
                in_use_flag = {true};
            }

            inline ~progressive_binary_reader() noexcept {
                writer.shift_left(reader.offset());
                in_use_flag = {false};
                TDSL_DEBUG_PRINTLN("netbuf: [consumed `%zu`, inuse `%zu`, free `%zu`]",
                                   reader.offset(), reader.remaining_bytes(),
                                   writer.remaining_bytes());
            }

            inline TDSL_NODISCARD binary_reader_type * operator->() noexcept {
                return &reader;
            }

            inline TDSL_NODISCARD binary_reader_type & operator*() noexcept {
                return reader;
            }

        private:
            binary_writer_type & writer;
            binary_reader_type reader{};
            bool & in_use_flag;
        };

        /**
         *
         *
         */
        struct progressive_binary_writer {
            inline progressive_binary_writer(binary_writer_type & w, bool & iuf) :
                writer(w), in_use_flag(iuf) {
                if (in_use_flag) {
                    TDSL_ASSERT_MSG(false, "Buffer object is already in use!");
                    TDSL_TRAP;
                }
                in_use_flag = {true};
            }

            inline ~progressive_binary_writer() noexcept {
                in_use_flag = {false};
            }

            inline TDSL_NODISCARD binary_writer_type * operator->() noexcept {
                return &writer;
            }

        private:
            binary_writer_type & writer;
            bool & in_use_flag;
        };

        /**
         * Get the reader object
         *
         * @return progressive_binary_reader
         */
        inline TDSL_NODISCARD auto get_reader() noexcept -> progressive_binary_reader {
            return progressive_binary_reader{*this, in_use};
        }

        /**
         * Get the writer object
         *
         * @return progressive_binary_writer
         */
        inline TDSL_NODISCARD auto get_writer() noexcept -> progressive_binary_writer {
            return progressive_binary_writer{*this, in_use};
        }

        /**
         * Get the underlying view object
         *
         * @return tdsl::byte_view
         */
        inline TDSL_NODISCARD auto get_underlying_view() const noexcept -> tdsl::byte_view {
            return binary_writer_type::underlying_view();
        }

    private:
        bool in_use = {false};
    };

} // namespace tdsl

#endif