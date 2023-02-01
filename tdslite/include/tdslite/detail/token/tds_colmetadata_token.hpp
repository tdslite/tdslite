/**
 * ____________________________________________________
 *
 * @file   tds_colmetadata_token.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   09.08.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TOKEN_TDS_COLMETADATA_TOKEN_HPP
#define TDSL_DETAIL_TOKEN_TDS_COLMETADATA_TOKEN_HPP

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
        tdsl::uint16_t user_type              = {0};
        tdsl::uint16_t flags                  = {0};
        /* Data type of the column */
        detail::e_tds_data_type type          = {static_cast<detail::e_tds_data_type>(0)};
        /* Length of the name of the column */
        tdsl::uint8_t colname_length_in_chars = {0};

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
            } ps = {};    // types with precision and scale
        } typeprops = {}; // type-specific properties
    };

    struct tds_colmetadata_token : public util::noncopyable {
        tdsl::span<tds_column_info> columns         = {};
        tdsl::span<tdsl::u16char_view> column_names = {};

        // --------------------------------------------------------------------------------

        /**
         * Default constructor
         */
        tds_colmetadata_token() noexcept            = default;

        // --------------------------------------------------------------------------------

        /**
         * Move constructor
         *
         * @param [in] other Object to move from
         */
        tds_colmetadata_token(tds_colmetadata_token && other) noexcept {
            if (this != &other) {
                maybe_release_resources();
                columns            = other.columns;
                column_names       = other.column_names;
                other.columns      = {};
                other.column_names = {};
            }
        }

        // --------------------------------------------------------------------------------

        /**
         * Move assignment
         *
         * @param [in] other Object to move from
         *
         * @return tds_colmetadata_token& self
         */
        tds_colmetadata_token & operator=(tds_colmetadata_token && other) noexcept {
            if (this != &other) {
                maybe_release_resources();
                columns            = other.columns;
                column_names       = other.column_names;
                other.columns      = {};
                other.column_names = {};
            }
            return *this;
        }

        // --------------------------------------------------------------------------------

        /**
         * Destructor
         */
        ~tds_colmetadata_token() noexcept {
            maybe_release_resources();
        }

        // --------------------------------------------------------------------------------

        inline operator bool() const noexcept {
            return columns;
        }

        // --------------------------------------------------------------------------------

        /**
         * Allocate space for columns.
         *
         * @param [in] col_count Column count
         *
         * @return true if allocation successful, false otherwise.
         */
        bool allocate_colinfo_array(tdsl::uint16_t col_count) noexcept {

            auto calloc = tds_allocator<tds_column_info>::create_n(col_count);
            if (calloc) {
                columns = tdsl::span<tdsl::tds_column_info>{calloc, calloc + col_count};
            }
            return not(nullptr == calloc);
        }

        // --------------------------------------------------------------------------------

        /**
         * Allocate space for column name pointers.
         *
         * @param [in] col_count Column count
         *
         * @return true if allocation successful, false otherwise.
         */
        bool allocate_column_name_array(tdsl::uint16_t col_count) noexcept {
            // Allocate N u16char_spans
            auto colname_arr = tds_allocator<u16char_view>::create_n(col_count);
            if (colname_arr) {
                column_names = tdsl::span<tdsl::u16char_view>{colname_arr, colname_arr + col_count};
            }
            return not(nullptr == colname_arr);
        }

        // --------------------------------------------------------------------------------

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
        bool set_column_name(tdsl::uint16_t index, byte_view name) noexcept {
            TDSL_ASSERT(index < columns.size());

            if (not name || not column_names) {
                return false;
            }

            const auto name_u16 = name.rebind_cast<const char16_t>();
            TDSL_ASSERT_MSG(name_u16.size_bytes() == name.size_bytes(),
                            "The raw column name bytes has odd size_bytes(), which is 'odd'"
                            "Column names are UCS-2 encoded so they must always have even size.");

            // alloc N char16_t's
            auto alloc = tds_allocator<char16_t>::allocate(name_u16.size());
            if (alloc) {
                column_names [index] = tdsl::u16char_view{alloc, alloc + name_u16.size()};
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

    private:
        /**
         * Release dynamically allocated resources, if any.
         */
        void maybe_release_resources() noexcept {
            if (columns) {
                tds_allocator<tds_column_info>::destroy_n(columns.data(), columns.size());
                columns = {};
            }
            if (column_names) {
                for (auto & colname : column_names) {
                    if (colname) {
                        // dealloc N char16_t's
                        tds_allocator<char16_t>::deallocate(const_cast<char16_t *>(colname.data()),
                                                            colname.size());
                        colname = {};
                    }
                }
                // free N spans
                tds_allocator<u16char_view>::destroy_n(column_names.data(), column_names.size());
                column_names = {};
            }
        }
    };

} // namespace tdsl

#endif