/**
 * _________________________________________________
 *
 * @file   ut_tdsl_field.cpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#include <tdslite/detail/tdsl_field.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using uut_t = tdsl::tdsl_field;

struct tdsl_field_fixture : public ::testing::Test {
    uut_t field{};
};

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_uint8) {
    tdsl::uint8_t buf [1] = {0x01};
    field                 = buf;
    EXPECT_EQ(field.as<tdsl::uint8_t>(), tdsl::uint8_t{1});
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_uint16) {
    tdsl::uint8_t buf [2] = {0x01, 0x02};
    field                 = buf;
    EXPECT_EQ(field.as<tdsl::uint16_t>(), tdsl::uint16_t{513});
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_uint32) {
    tdsl::uint8_t buf [4] = {0x01, 0x02, 0x03, 0x04};
    field                 = buf;
    EXPECT_EQ(field.as<tdsl::uint32_t>(), tdsl::uint32_t{67305985});
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_uint64) {
    tdsl::uint8_t buf [8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    field                 = buf;
    EXPECT_EQ(field.as<tdsl::uint64_t>(), tdsl::uint64_t{578437695752307201});
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_string_view) {
    constexpr tdsl::uint8_t buf [14] = {0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73,
                                        0x20, 0x61, 0x20, 0x74, 0x65, 0x73, 0x74};
    field                            = buf;
    constexpr const char str []      = "this is a test";
    tdsl::char_view expected_span{str, sizeof(str) - 1};
    ASSERT_THAT(field.as<tdsl::char_view>(), testing::ElementsAreArray(expected_span));
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_u16char_view) {
    constexpr tdsl::uint8_t buf [28] = {0x74, 0x00, 0x68, 0x00, 0x69, 0x00, 0x73, 0x00, 0x20, 0x00,
                                        0x69, 0x00, 0x73, 0x00, 0x20, 0x00, 0x61, 0x00, 0x20, 0x00,
                                        0x74, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00};
    field                            = buf;
    constexpr const char16_t str []  = u"this is a test";
    tdsl::u16char_view expected_span{str, (sizeof(str) / sizeof(char16_t)) - 1};
    auto result = field.as<tdsl::u16char_view>();
    ASSERT_EQ(result.size(), 14);
    ASSERT_EQ(result.size_bytes(), 28);
    ASSERT_THAT(result, testing::ElementsAreArray(expected_span));
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_u32string_view) {
    constexpr tdsl::uint8_t buf [56] = {
        0x74, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x73, 0x00,
        0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00,
        0x20, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x74, 0x00,
        0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00};
    field                           = buf;
    constexpr const char32_t str [] = U"this is a test";
    tdsl::u32char_view expected_span{str, (sizeof(str) / sizeof(char32_t)) - 1};
    auto result = field.as<tdsl::u32char_view>();
    ASSERT_EQ(result.size(), 14);
    ASSERT_EQ(result.size_bytes(), 56);
    ASSERT_THAT(result, testing::ElementsAreArray(expected_span));
}
