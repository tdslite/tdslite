/**
 * _________________________________________________
 *
 * @file   tdsl_colmetadata_token.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   09.08.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_string_view.hpp>
#include <tdslite/util/tdsl_noncopyable.hpp>
#include <tdslite/detail/tdsl_data_type.hpp>
#include <tdslite/detail/tdsl_allocator.hpp>

namespace tdsl {

    // Packed into 12-byte layout to make
    // most of the space useful.
    struct tds_column_info {
        /* User-defined type value */
        tdsl::uint16_t user_type;
        tdsl::uint16_t flags;
        /* Data type of the column */
        detail::e_tds_data_type type;
        /* Length of the name of the column */
        tdsl::uint8_t colname_length_in_chars;

        union {
            struct {
                tdsl::uint32_t length;
            } u32l; // types with variable length of unsigned 32-bit size

            struct {
                tdsl::uint16_t length;
                tdsl::uint8_t _unused [2];
            } u16l; // types with variable length of unsigned 16-bit size

            struct {
                tdsl::uint8_t length;
                tdsl::uint8_t _unused [3];
            } u8l; // types with variable length of unsigned 8-bit size

            struct {
                tdsl::uint8_t length; // this is the actual length, not the length's length.
                tdsl::uint8_t _unused [3];
            } fixed; // types with fixed length

            struct {
                tdsl::uint8_t length;
                tdsl::uint8_t precision;
                tdsl::uint8_t scale;
                tdsl::uint8_t _unused [1];
            } ps{};  // types with precision and scale
        } typeprops; // type-specific properties
    };

    struct tds_colmetadata_token : public util::noncopyable {
        tdsl::uint16_t column_count{0};
        tds_column_info * columns = {nullptr};
        tdsl::span<tdsl::u16char_span> column_names;
        // char16_t ** column_names  = {nullptr};

        tds_colmetadata_token() = default;

        inline bool is_valid() const noexcept {
            return column_count > 0 && not(nullptr == columns);
        }

        /**
         * Allocate space for columns.
         *
         * @param [in] col_count Column count
         *
         * @return true if allocation successful, false otherwise.
         */
        bool allocate_colinfo_array(tdsl::uint16_t col_count) {
            if ((columns = tds_allocator<tds_column_info>::allocate(col_count))) {
                column_count = col_count;
            }
            return not(nullptr == columns);
        }

        /**
         * Allocate space for column name pointers.
         *
         * @param [in] col_count Column count
         *
         * @return true if allocation successful, false otherwise.
         */
        bool allocate_column_name_array(tdsl::uint16_t col_count) {

            // Allocate N u16char_spans
            auto colname_arr = tds_allocator<u16char_span>::allocate(col_count);
            if (colname_arr) {
                column_names = tdsl::span<tdsl::u16char_span>{colname_arr, colname_arr + col_count};
                // zero-initialize all columns
                for (auto & columnnspan : column_names) {
                    columnnspan = tdsl::u16char_span{/*begin=*/nullptr, /*end=*/nullptr};
                }
            }
            return not(nullptr == colname_arr);
        }

        /**
         * Set the name of the column # @p index to @p name.
         *
         * This function allocates at least @p name.size_bytes() bytes
         * memory and copies @p name into allocated memory.
         *
         * @param [in] index Column index
         * @param [in] name Name value
         * @return true if set successful, false if memory allocation failed
         */
        bool set_column_name(tdsl::uint16_t index, tdsl::span<const tdsl::uint8_t> name) {
            TDSL_ASSERT(index < column_count);
            const auto name_u16 = name.rebind_cast<const char16_t>();
            TDSL_ASSERT_MSG(name_u16.size_bytes() == name.size_bytes(),
                            "The raw column name bytes has odd size_bytes(), which is 'odd'"
                            "Column names are UCS-2 encoded so they must always have even size.");

            // alloc N char16_t's
            auto alloc = tds_allocator<char16_t>::allocate(name_u16.size());
            if (alloc) {
                column_names [index] = tdsl::u16char_span{alloc, alloc + name_u16.size()};
                // Copy column name data to allocated space
                for (tdsl::uint32_t i = 0; i < name.size_bytes(); i += 2) {
                    char16_t cv        = name [i] | name [i + 1] << 8;
                    *(alloc + (i / 2)) = cv;
                }
                TDSL_ASSERT(column_names [index].data() != nullptr);
                TDSL_ASSERT(column_names [index].size() > 0);
            }

            return not(nullptr == alloc);
        }

        inline void reset() noexcept {
            if (columns) {
                tds_allocator<tds_column_info>::deallocate(columns, column_count);
                columns = {nullptr};
            }
            if (column_names) {

                for (auto & colname : column_names) {
                    if (colname) {
                        // dealloc N char16_t's
                        tds_allocator<char16_t>::deallocate(const_cast<char16_t *>(colname.data()),
                                                            colname.size());
                        colname = tdsl::u16char_span{/*begin=*/nullptr, /*end=*/nullptr};
                    }
                }

                // free N spans
                tds_allocator<tdsl::u16char_span>::deallocate(column_names.data(),
                                                              column_names.size());
                column_names = tdsl::span<tdsl::u16char_span>{/*begin=*/nullptr, /*end=*/nullptr};
            }
            column_count = 0;
        }

        ~tds_colmetadata_token() {
            reset();
        }

        tds_colmetadata_token(tds_colmetadata_token && other) noexcept {
            column_count       = other.column_count;
            columns            = other.columns;
            column_names       = other.column_names;
            other.column_count = 0;
            other.columns      = {nullptr};
            other.column_names = tdsl::span<tdsl::u16char_span>{/*begin=*/nullptr, /*end=*/nullptr};
        }

        tds_colmetadata_token & operator=(tds_colmetadata_token && other) noexcept {
            column_count       = other.column_count;
            columns            = other.columns;
            column_names       = other.column_names;
            other.column_count = 0;
            other.columns      = {nullptr};
            other.column_names = tdsl::span<tdsl::u16char_span>{/*begin=*/nullptr, /*end=*/nullptr};
            return *this;
        }
    };

} // namespace tdsl