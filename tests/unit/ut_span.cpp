/**
 * _________________________________________________
 * Unit tests for span<> type
 *
 * @file   ut_span.cpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   14.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#include <tdslite/util/tdsl_span.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

// --------------------------------------------------------------------------------

TEST(span, default_construct) {
    tdsl::byte_span buf_span{};
    EXPECT_EQ(buf_span.data(), nullptr);
    EXPECT_EQ(buf_span.size_bytes(), 0);
    EXPECT_FALSE(buf_span);
}

// --------------------------------------------------------------------------------

TEST(span, construct_from_fs_array) {
    tdsl::uint8_t buf [4];
    tdsl::byte_span buf_span{buf};
    EXPECT_EQ(buf_span.data(), buf);
    EXPECT_EQ(buf_span.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span.begin(), buf);
    EXPECT_EQ(buf_span.end(), &buf [4]);
    EXPECT_TRUE(buf_span);
}

// --------------------------------------------------------------------------------

TEST(span, construct_from_buf_with_size) {
    tdsl::uint8_t buf [4];
    tdsl::byte_span buf_span{buf, sizeof(buf)};
    EXPECT_EQ(buf_span.data(), buf);
    EXPECT_EQ(buf_span.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span.begin(), buf);
    EXPECT_EQ(buf_span.end(), &buf [4]);
    EXPECT_TRUE(buf_span);
}

// --------------------------------------------------------------------------------

TEST(span, construct_from_buf_with_begin_end) {
    tdsl::uint8_t buf [4];
    tdsl::byte_span buf_span{buf, &buf [4]};
    EXPECT_EQ(buf_span.data(), buf);
    EXPECT_EQ(buf_span.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span.begin(), buf);
    EXPECT_EQ(buf_span.end(), &buf [4]);
    EXPECT_TRUE(buf_span);
}

// --------------------------------------------------------------------------------

TEST(span, copy_construct) {
    tdsl::uint8_t buf [4];
    tdsl::byte_span buf_span{buf};
    tdsl::byte_span buf_span_cc{buf_span};
    EXPECT_EQ(buf_span, buf_span_cc);
    EXPECT_EQ(buf_span.data(), buf);
    EXPECT_EQ(buf_span.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span.begin(), buf);
    EXPECT_EQ(buf_span.end(), &buf [4]);
    EXPECT_TRUE(buf_span);

    EXPECT_EQ(buf_span_cc.data(), buf);
    EXPECT_EQ(buf_span_cc.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span_cc.begin(), buf);
    EXPECT_EQ(buf_span_cc.end(), &buf [4]);
    EXPECT_TRUE(buf_span_cc);
}

// --------------------------------------------------------------------------------

TEST(span, copy_assign) {
    tdsl::uint8_t buf [4];
    tdsl::byte_span buf_span{buf};
    tdsl::byte_span buf_span_cc = buf_span;
    EXPECT_EQ(buf_span, buf_span_cc);
    EXPECT_EQ(buf_span.data(), buf);
    EXPECT_EQ(buf_span.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span.begin(), buf);
    EXPECT_EQ(buf_span.end(), &buf [4]);
    EXPECT_TRUE(buf_span);

    EXPECT_EQ(buf_span_cc.data(), buf);
    EXPECT_EQ(buf_span_cc.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span_cc.begin(), buf);
    EXPECT_EQ(buf_span_cc.end(), &buf [4]);
    EXPECT_TRUE(buf_span_cc);
}

// --------------------------------------------------------------------------------

TEST(span, move_construct) {
    tdsl::uint8_t buf [4];
    tdsl::byte_span buf_span{buf};
    tdsl::byte_span buf_span_mc{TDSL_MOVE(buf_span)};
    EXPECT_NE(buf_span, buf_span_mc);
    EXPECT_EQ(buf_span.data(), nullptr);
    EXPECT_EQ(buf_span.size_bytes(), 0);
    EXPECT_EQ(buf_span.begin(), nullptr);
    EXPECT_EQ(buf_span.end(), nullptr);
    EXPECT_FALSE(buf_span);

    EXPECT_EQ(buf_span_mc.data(), buf);
    EXPECT_EQ(buf_span_mc.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span_mc.begin(), buf);
    EXPECT_EQ(buf_span_mc.end(), &buf [4]);
    EXPECT_TRUE(buf_span_mc);
}

// --------------------------------------------------------------------------------

TEST(span, move_assign) {
    tdsl::uint8_t buf [4];
    tdsl::byte_span buf_span{buf};
    tdsl::byte_span buf_span_mc = TDSL_MOVE(buf_span);
    EXPECT_NE(buf_span, buf_span_mc);
    EXPECT_EQ(buf_span.data(), nullptr);
    EXPECT_EQ(buf_span.size_bytes(), 0);
    EXPECT_EQ(buf_span.begin(), nullptr);
    EXPECT_EQ(buf_span.end(), nullptr);
    EXPECT_FALSE(buf_span);

    EXPECT_EQ(buf_span_mc.data(), buf);
    EXPECT_EQ(buf_span_mc.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span_mc.begin(), buf);
    EXPECT_EQ(buf_span_mc.end(), &buf [4]);
    EXPECT_TRUE(buf_span_mc);
}

// --------------------------------------------------------------------------------

TEST(span, char_view_construct) {
    tdsl::char_view buf_span{"test"};
    (void) buf_span;
}

// --------------------------------------------------------------------------------

TEST(span, char_view_assign) {
    tdsl::char_view buf_span{};
    EXPECT_NO_THROW({ buf_span = "test"; });
    EXPECT_EQ(buf_span.size(), 5);
}

// --------------------------------------------------------------------------------

TEST(span, shift_left_1) {
    tdsl::uint8_t buf []            = {0x01, 0x02, 0x03, 0x04, 0x05};
    tdsl::uint8_t expected_buf_1 [] = {0x03, 0x04, 0x05, 0x00, 0x00};
    tdsl::uint8_t expected_buf_2 [] = {0x00, 0x00, 0x00, 0x00, 0x00};
    tdsl::byte_span buf_span{buf};
    EXPECT_EQ(3, buf_span.shift_left(/*count=*/2));
    ASSERT_THAT(buf, testing::ElementsAreArray(expected_buf_1));

    EXPECT_EQ(2, buf_span.shift_left(/*count=*/3));
    ASSERT_THAT(buf, testing::ElementsAreArray(expected_buf_2));
    EXPECT_EQ(0, buf_span.shift_left(/*count=*/5));
}

// --------------------------------------------------------------------------------

TEST(span, shift_left_2) {
    tdsl::uint8_t buf [] = {0x01, 0x02, 0x03, 0x04, 0x05};
    tdsl::byte_span buf_span{buf};
    for (tdsl::uint32_t i = 1; i <= sizeof(buf); i++) {
        EXPECT_EQ(sizeof(buf) - i, buf_span.shift_left(i));
    }
    tdsl::byte_view bv{buf};
}

// --------------------------------------------------------------------------------

TEST(span, shift_left_3) {
    std::vector<tdsl::uint8_t> buf;
    buf.resize(8192);
    tdsl::byte_span buf_span{buf.data(), static_cast<tdsl::uint32_t>(buf.size())};
    for (tdsl::uint32_t i = 0; i < buf.size(); i++) {
        EXPECT_EQ(buf.size() - i, buf_span.shift_left(i));
    }
}

// --------------------------------------------------------------------------------

TEST(span, shift_left_oversize) {
    tdsl::uint8_t expected_buf [8192] = {};
    std::vector<tdsl::uint8_t> buf{};
    buf.resize(8192);
    std::fill(buf.begin(), buf.end(), 0xFF);
    tdsl::byte_span buf_span{buf.data(), static_cast<tdsl::uint32_t>(buf.size())};
    EXPECT_EQ(buf_span.shift_left(buf.size() + 1), 0);
    ASSERT_THAT(buf, testing::ElementsAreArray(expected_buf));
}