/**
 * ____________________________________________________
 * Unit tests for sql_parameter and sql_parameter_binding
 * types
 *
 * @file   ut_sql_parameter.cpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   10.01.2023
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <tdslite/detail/tdsl_sql_parameter.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

template <tdsl::detail::e_tds_data_type DT>
using uut_t     = tdsl::detail::sql_parameter<DT>;
using data_type = tdsl::detail::e_tds_data_type;

// --------------------------------------------------------------------------------

class sql_param_fixture : public ::testing::Test {};

// --------------------------------------------------------------------------------

TEST_F(sql_param_fixture, construct) {
    ASSERT_NO_THROW({
        uut_t<tdsl::detail::e_tds_data_type::INT1TYPE> v{};
        EXPECT_EQ(v, tdsl::uint8_t{});
    });
}

// --------------------------------------------------------------------------------

TEST_F(sql_param_fixture, construct_with_value) {
    ASSERT_NO_THROW({
        uut_t<tdsl::detail::e_tds_data_type::INT1TYPE> v{tdsl::uint8_t{6}};
        EXPECT_EQ(v, 6);
    });
}

// --------------------------------------------------------------------------------

TEST_F(sql_param_fixture, copy_assign) {
    uut_t<tdsl::detail::e_tds_data_type::INT1TYPE> v = tdsl::uint8_t{6};
    ASSERT_EQ(v, 6);
}

// --------------------------------------------------------------------------------

TEST_F(sql_param_fixture, assign) {
    uut_t<tdsl::detail::e_tds_data_type::INT1TYPE> v{tdsl::uint8_t{6}};
    ASSERT_EQ(v, 6);
    v = 7;
    ASSERT_EQ(v, 7);
}

// --------------------------------------------------------------------------------

TEST_F(sql_param_fixture, assign_to_backing_type) {
    ASSERT_NO_THROW({
        uut_t<tdsl::detail::e_tds_data_type::INT1TYPE> v{17};
        tdsl::uint8_t v2 = v;
        EXPECT_EQ(v2, tdsl::uint8_t{17});
    });
}

// --------------------------------------------------------------------------------

TEST_F(sql_param_fixture, arithmetic_add) {
    uut_t<tdsl::detail::e_tds_data_type::INT1TYPE> v{tdsl::uint8_t{6}};
    ASSERT_EQ(v, 6);
    v = 7 + v;
    ASSERT_EQ(v, 13);
}

// --------------------------------------------------------------------------------

TEST_F(sql_param_fixture, arithmetic_sub) {
    uut_t<tdsl::detail::e_tds_data_type::INT1TYPE> v{tdsl::uint8_t{6}};
    ASSERT_EQ(v, 6);
    v = 7 - v;
    ASSERT_EQ(v, 1);
}

// --------------------------------------------------------------------------------

TEST_F(sql_param_fixture, arithmetic_self) {
    uut_t<tdsl::detail::e_tds_data_type::INT1TYPE> v{tdsl::uint8_t{6}};
    ASSERT_EQ(v, 6);
    v = v + 5;
    v = v + v;
    ASSERT_EQ(22, v);
}

// --------------------------------------------------------------------------------

TEST_F(sql_param_fixture, param_binding_int1type) {
    uut_t<tdsl::detail::e_tds_data_type::INT1TYPE> v{tdsl::uint8_t{6}};
    tdsl::detail::sql_parameter_binding binding = v;
    ASSERT_EQ(binding.type, tdsl::detail::e_tds_data_type::INT1TYPE);
    ASSERT_EQ(binding.type_size, sizeof(tdsl::uint8_t));
    ASSERT_EQ(binding.value.size_bytes(), sizeof(tdsl::uint8_t));
    ASSERT_EQ(*binding.value.data(), tdsl::uint8_t{6});
}

// --------------------------------------------------------------------------------

// googletest's typed test suite macros does not play well with
// non-type template parameters, so we have to wrap our non-type
// template parameter into a struct that'll act as an intermediate
// type template parameter.

template <tdsl::detail::e_tds_data_type V>
struct hack {
    static constexpr tdsl::detail::e_tds_data_type value = {V};
};

template <tdsl::detail::e_tds_data_type V>
constexpr tdsl::detail::e_tds_data_type hack<V>::value;

// --------------------------------------------------------------------------------

template <typename T>
class sql_param_binding_fixture : public ::testing::Test {};

// --------------------------------------------------------------------------------

TYPED_TEST_SUITE_P(sql_param_binding_fixture);

// --------------------------------------------------------------------------------

TYPED_TEST_P(sql_param_binding_fixture, param_binding_validation) {
    // Inside a test, refer to TypeParam to get the type parameter.
    using traits = tdsl::detail::sql_param_traits<TypeParam::value>;
    uut_t<TypeParam::value> v{typename traits::type{1}};

    tdsl::detail::sql_parameter_binding binding = v;
    ASSERT_EQ(binding.type, TypeParam::value);
    ASSERT_EQ(binding.type_size, sizeof(typename traits::type));
    ASSERT_TRUE(binding.value);
    ASSERT_EQ(binding.value.size_bytes(), sizeof(typename traits::type));
    ASSERT_EQ(*binding.value.data(), typename traits::type{1});
}

// --------------------------------------------------------------------------------

REGISTER_TYPED_TEST_SUITE_P(sql_param_binding_fixture, param_binding_validation);

// --------------------------------------------------------------------------------

using types =
    ::testing::Types<hack<data_type::BITTYPE>, hack<data_type::INT1TYPE>, hack<data_type::INT2TYPE>,
                     hack<data_type::INT4TYPE>, hack<data_type::INT8TYPE>>;

INSTANTIATE_TYPED_TEST_SUITE_P(t, sql_param_binding_fixture, types);

// --------------------------------------------------------------------------------