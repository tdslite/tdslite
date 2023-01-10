/**
 * _________________________________________________
 *
 * @file   tdsl_sql_parameter.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   10.01.2023
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#ifndef TDSL_DETAIL_TDSL_SQL_PARAMETER_HPP
#define TDSL_DETAIL_TDSL_SQL_PARAMETER_HPP

#include <tdslite/detail/tdsl_data_type.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <tdslite/util/tdsl_byte_swap.hpp>

namespace tdsl { namespace detail {

    struct sql_parameter_binding {
        e_tds_data_type type;
        tdsl::byte_view value;
        tdsl::uint32_t type_size{}; // required for some types only
    };

    // --------------------------------------------------------------------------------

    template <e_tds_data_type>
    struct sql_param_traits;

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::BITTYPE> {
        using type = bool;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::INT1TYPE> {
        using type = tdsl::uint8_t;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::INT2TYPE> {
        using type = tdsl::int16_t;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::INT4TYPE> {
        using type = tdsl::int32_t;
    };

    // --------------------------------------------------------------------------------

    template <>
    struct sql_param_traits<e_tds_data_type::INT8TYPE> {
        using type = tdsl::int64_t;
    };

    // --------------------------------------------------------------------------------

    template <e_tds_data_type DTYPE>
    struct sql_parameter {

        // All integer types are represented in reverse byte order (little-endian) unless otherwise
        // specified.

        using param_traits = sql_param_traits<DTYPE>;
        using backing_type = typename param_traits::type;

        inline sql_parameter() : value{} {}

        inline sql_parameter(backing_type value) : value(native_to_le(value)) {}

        /**
         * Act as backing type for const operations.
         *
         * @return backing_type backing type value in native
         *         byte order
         */
        inline operator backing_type() const noexcept {
            return le_to_native(value);
        }

        /**
         * Cast operator to sql_paramater_binding
         */
        inline operator sql_parameter_binding() const noexcept {
            sql_parameter_binding param{};
            param.type      = DTYPE;
            param.type_size = sizeof(backing_type); // this might be wrong
            param.value     = to_bytes();
            return param;
        }

    private:
        /**
         * Get @ref value as bytes
         */
        inline tdsl::byte_view to_bytes() const noexcept {
            return tdsl::byte_view{reinterpret_cast<const tdsl::uint8_t *>(&value),
                                   sizeof(backing_type)};
        }

        // backing type for parameter, stored as
        // little-endian byte order regardless of
        // native byte order.
        backing_type value;
    };

    using sql_parameter_tinyint  = sql_parameter<e_tds_data_type::INT1TYPE>;
    using sql_parameter_smallint = sql_parameter<e_tds_data_type::INT2TYPE>;
    using sql_parameter_int      = sql_parameter<e_tds_data_type::INT4TYPE>;
    using sql_parameter_bigint   = sql_parameter<e_tds_data_type::INT8TYPE>;

}} // namespace tdsl::detail

#endif