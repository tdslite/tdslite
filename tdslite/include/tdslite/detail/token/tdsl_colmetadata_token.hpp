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

#include <tdslite/detail/tdsl_data_type.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_string_view.hpp>
#include <tdslite/util/tdsl_noncopyable.hpp>

#include <cstdlib>
#include <new>

namespace tdsl {

    // Carefully packed into 12-byte layout to make
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
                tdsl::uint8_t precision;
                tdsl::uint8_t scale;
                tdsl::uint8_t _unused [2];
            } ps{};  // types with precision and scale
        } typeprops; // type-specific properties
    };

    // TODO: move this to a saner place
    inline void * tds_allocate(unsigned long n_bytes) noexcept {
        return std::malloc(n_bytes);
    }

    inline void tds_deallocate(void * p, unsigned long n_bytes) noexcept {
        (void) n_bytes;
        std::free(p);
    }

    template <typename T>
    struct tds_allocator {
        static T * allocate(tdsl::uint32_t n_elems) {
            void * mem = tds_allocate(n_elems * sizeof(T));
            if (nullptr == mem) {
                return nullptr;
            }

            // Invoke placement new for each element
            T * storage = static_cast<T *>(mem);
            // use placement new here
            return storage;
        }

        static void deallocate(T * p, tdsl::uint32_t n_elems) {
            // call destructor if class type

            tds_deallocate(p, sizeof(T) * n_elems);
        }

        template <typename Q = T, traits::enable_if_class<Q> = true>
        static void construct(Q * storage, tdsl::uint32_t n_elems) {
            for (tdsl::uint32_t i = 0; i < n_elems; i++) {
                new (storage + i) Q();
            }
        }

        template <typename Q = T, traits::enable_if_class<Q> = true>
        static void destruct(Q * storage, tdsl::uint32_t n_elems) {
            for (tdsl::uint32_t i = 0; i < n_elems; i++) {
                storage [i].~Q();
            }
        }

    private:
        // noop
        template <typename Q>
        static void construct(Q *, tdsl::uint32_t) {}

        // noop
        template <typename Q>
        static void destruct(Q *, tdsl::uint32_t) {}
    };

    struct tds_colmetadata_token : public util::noncopyable {
        tdsl::uint16_t column_count{0};
        tds_column_info * columns = {nullptr};
        char16_t ** column_names  = {nullptr};

        tds_colmetadata_token()   = default;

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
            return not(nullptr == (column_names = tds_allocator<char16_t *>::allocate(col_count)));
        }

        /**
         * Set the name of the column # @p index to @p name.
         *
         * This function allocates at least @p name.size_bytes() bytes
         * memory and copies @p name into allocated memory.
         *
         * @param index
         * @param name
         * @return true
         * @return false
         */
        bool set_column_name(tdsl::uint16_t index, tdsl::span<const tdsl::uint8_t> name) {
            TDSLITE_ASSERT(index < column_count);
            column_names [index] = tds_allocator<char16_t>::allocate(name.size_bytes() / 2);
            return not(nullptr == column_names [index]);
        }

        ~tds_colmetadata_token() {
            if (columns) {
                tds_allocator<tds_column_info>::deallocate(columns, column_count);
                columns = {nullptr};
            }
            if (column_names) {

                for (int i = 0; i < column_count; i++) {
                    if (not(nullptr == column_names [i])) {
                        tds_allocator<char16_t>::deallocate(column_names [i], 0);
                    }
                }
                tds_allocator<char16_t *>::deallocate(column_names, column_count);
                column_names = {nullptr};
            }
            column_count = 0;
        }

        tds_colmetadata_token(tds_colmetadata_token && other) noexcept {
            column_count       = other.column_count;
            columns            = other.columns;
            column_names       = other.column_names;
            other.column_count = 0;
            other.columns      = {nullptr};
            other.column_names = {nullptr};
        }

        tds_colmetadata_token & operator=(tds_colmetadata_token && other) noexcept {
            column_count       = other.column_count;
            columns            = other.columns;
            column_names       = other.column_names;
            other.column_count = 0;
            other.columns      = {nullptr};
            other.column_names = {nullptr};
            return *this;
        }
    };

} // namespace tdsl