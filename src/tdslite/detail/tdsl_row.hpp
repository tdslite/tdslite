/**
 * ____________________________________________________
 * Result set row type
 *
 * @file   tdsl_row.hpp
 * @author mkg <me@mustafagilor.com>
 * @date   29.09.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TDSL_ROW_HPP
#define TDSL_DETAIL_TDSL_ROW_HPP

#include <tdslite/detail/tdsl_allocator.hpp>
#include <tdslite/detail/tdsl_field.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_expected.hpp>
#include <tdslite/util/tdsl_noncopyable.hpp>

namespace tdsl {

    /**
     * A type that represents a row in a result set.
     * Composed of N fields, where N is the field count.
     */
    struct tdsl_row : util::noncopyable {
        enum class e_tdsl_row_make_err
        {
            MEM_ALLOC = -1
        };
        using field_allocator_t = tds_allocator<tdsl_field>;
        using fields_type_t     = tdsl::span<tdsl_field>;
        using make_result_t     = tdsl::expected<tdsl_row, e_tdsl_row_make_err>;

        // --------------------------------------------------------------------------------

        inline TDSL_NODISCARD auto begin() const noexcept -> fields_type_t::iterator {
            return fields.begin();
        }

        // --------------------------------------------------------------------------------

        inline TDSL_NODISCARD auto end() const noexcept -> fields_type_t::iterator {
            return fields.end();
        }

        // --------------------------------------------------------------------------------

        inline TDSL_NODISCARD auto cbegin() const noexcept -> fields_type_t::const_iterator {
            return fields.cbegin();
        }

        // --------------------------------------------------------------------------------

        inline TDSL_NODISCARD auto cend() const noexcept -> fields_type_t::const_iterator {
            return fields.cend();
        }

        // --------------------------------------------------------------------------------

        inline TDSL_NODISCARD auto operator[](fields_type_t::size_type index) const noexcept
            -> fields_type_t::reference {
            return fields [index];
        }

        // --------------------------------------------------------------------------------

        inline TDSL_NODISCARD auto size() const noexcept -> fields_type_t::size_type {
            return fields.size();
        }

        // --------------------------------------------------------------------------------

        /**
         * Allocate space for @p n_col fields and make a row object
         *
         * @param n_col number of columns
         *
         * @return tdsl_row with n_col field on success
         * @return e_tdsl_row_make_err::FAILURE_MEM_ALLOC on failure
         */
        static inline TDSL_NODISCARD make_result_t make(tdsl::uint32_t n_col) {
            tdsl_field * fields = field_allocator_t::create_n(n_col);
            if (fields) {
                return tdsl_row(fields, n_col);
            }
            return make_result_t::unexpected(e_tdsl_row_make_err::MEM_ALLOC);
        }

        // --------------------------------------------------------------------------------

        tdsl_row(tdsl_row && other) noexcept {
            if (this != &other) {
                maybe_release_resources();
                fields       = other.fields;
                other.fields = {};
            }
        }

        // --------------------------------------------------------------------------------

        tdsl_row & operator=(tdsl_row && other) noexcept {
            if (this != &other) {
                maybe_release_resources();
                fields       = other.fields;
                other.fields = {};
            }
            return *this;
        }

        // --------------------------------------------------------------------------------

        ~tdsl_row() noexcept {
            maybe_release_resources();
        }

    private:
        fields_type_t fields = {};

        // --------------------------------------------------------------------------------

        /**
         * Private c-tor
         *
         * Takes ownership of @p fields
         *
         * Hint: use make() function
         * to make instances of row.
         *
         * @param fields Allocated fields
         * @param field_count Field count
         */
        explicit tdsl_row(tdsl_field * fields, tdsl::uint32_t field_count) noexcept :
            fields(fields, field_count) {}

        // --------------------------------------------------------------------------------

        void maybe_release_resources() noexcept {
            if (fields) {
                field_allocator_t::destroy_n(fields.data(), fields.size());
                fields = tdsl::span<tdsl_field>();
            }
        }
    };
} // namespace tdsl

#endif