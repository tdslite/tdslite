/**
 * _________________________________________________
 *
 * @file   tdsl_row.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   29.09.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/detail/tdsl_allocator.hpp>
#include <tdslite/detail/tdsl_field.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_expected.hpp>
#include <tdslite/util/tdsl_noncopyable.hpp>

namespace tdsl {

    struct tdsl_row : util::noncopyable {
        using field_allocator_t = tds_allocator<tdsl_field>;
        using fields_type_t     = tdsl::span<tdsl_field>;

        enum class e_tdsl_row_make_err
        {
            MEM_ALLOC = -1
        };

        inline auto begin() const noexcept -> fields_type_t::iterator {
            return fields.begin();
        }

        inline auto end() const noexcept -> fields_type_t::iterator {
            return fields.end();
        }

        inline auto cbegin() const noexcept -> fields_type_t::const_iterator {
            return fields.cbegin();
        }

        inline auto cend() const noexcept -> fields_type_t::const_iterator {
            return fields.cend();
        }

        inline auto operator[](fields_type_t::size_type index) const noexcept
            -> fields_type_t::reference {
            return fields [index];
        }

        inline auto size() const noexcept -> fields_type_t::size_type {
            return fields.size();
        }

        /**
         * Allocate space for @p n_col fields and make a row object
         *
         * @param n_col number of columns
         *
         * @return tdsl_row with n_col field on success
         * @return e_tdsl_row_make_err::FAILURE_MEM_ALLOC on failure
         */
        static inline tdsl::expected<tdsl_row, e_tdsl_row_make_err> make(tdsl::uint32_t n_col) {
            tdsl_field * fields = field_allocator_t::create_n(n_col);
            if (fields) {
                return TDSL_MOVE(
                    tdsl::expected<tdsl_row, e_tdsl_row_make_err>{tdsl_row(fields, n_col)});
            }
            return tdsl::unexpected<e_tdsl_row_make_err>(e_tdsl_row_make_err::MEM_ALLOC);
        }

        tdsl_row(tdsl_row && other) {
            fields       = other.fields;
            other.fields = tdsl::span<tdsl_field>();
        }

        tdsl_row & operator=(tdsl_row && other) {
            fields       = other.fields;
            other.fields = tdsl::span<tdsl_field>();
            return *this;
        }

        ~tdsl_row() {
            if (fields) {
                field_allocator_t::destroy_n(fields.data(), fields.size());
                fields = tdsl::span<tdsl_field>();
            }
        }

    private:
        fields_type_t fields;

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
        explicit tdsl_row(tdsl_field * fields, tdsl::uint32_t field_count) :
            fields(fields, field_count) {}
    };
} // namespace tdsl