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
#include <tdslite/util/tdsl_macrodef.hpp>
#include <gtest/gtest.h>

TEST(span, default_construct) {
    tdsl::span<tdsl::uint8_t> buf_span{};
    EXPECT_EQ(buf_span.data(), nullptr);
    EXPECT_EQ(buf_span.size_bytes(), 0);
    EXPECT_FALSE(buf_span);
}

TEST(span, construct_from_fs_array) {
    tdsl::uint8_t buf [4];
    tdsl::span<tdsl::uint8_t> buf_span{buf};
    EXPECT_EQ(buf_span.data(), buf);
    EXPECT_EQ(buf_span.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span.begin(), buf);
    EXPECT_EQ(buf_span.end(), &buf [4]);
    EXPECT_TRUE(buf_span);
}

TEST(span, construct_from_buf_with_size) {
    tdsl::uint8_t buf [4];
    tdsl::span<tdsl::uint8_t> buf_span{buf, sizeof(buf)};
    EXPECT_EQ(buf_span.data(), buf);
    EXPECT_EQ(buf_span.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span.begin(), buf);
    EXPECT_EQ(buf_span.end(), &buf [4]);
    EXPECT_TRUE(buf_span);
}

TEST(span, construct_from_buf_with_begin_end) {
    tdsl::uint8_t buf [4];
    tdsl::span<tdsl::uint8_t> buf_span{buf, &buf [4]};
    EXPECT_EQ(buf_span.data(), buf);
    EXPECT_EQ(buf_span.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span.begin(), buf);
    EXPECT_EQ(buf_span.end(), &buf [4]);
    EXPECT_TRUE(buf_span);
}

TEST(span, copy_construct) {
    tdsl::uint8_t buf [4];
    tdsl::span<tdsl::uint8_t> buf_span{buf};
    tdsl::span<tdsl::uint8_t> buf_span_cc{buf_span};
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

TEST(span, copy_assign) {
    tdsl::uint8_t buf [4];
    tdsl::span<tdsl::uint8_t> buf_span{buf};
    tdsl::span<tdsl::uint8_t> buf_span_cc = buf_span;
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

TEST(span, move_construct) {
    tdsl::uint8_t buf [4];
    tdsl::span<tdsl::uint8_t> buf_span{buf};
    tdsl::span<tdsl::uint8_t> buf_span_mc{TDSLITE_MOVE(buf_span)};
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

TEST(span, move_assign) {
    tdsl::uint8_t buf [4];
    tdsl::span<tdsl::uint8_t> buf_span{buf};
    tdsl::span<tdsl::uint8_t> buf_span_mc = TDSLITE_MOVE(buf_span);
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

TEST(span, string_view_construct) {
    tdsl::span<const char> buf_span{"test"};
    (void) buf_span;
}

TEST(span, string_view_assign) {
    tdsl::span<const char> buf_span{};
    EXPECT_NO_THROW({ buf_span = "test"; });
}