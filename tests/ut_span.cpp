/**
 * _________________________________________________
 * Unit tests for span<> type
 *
 * @file   ut_span.cpp
 * @author Mustafa Kemal GILOR <mgilor@nettsi.com>
 * @date   14.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#include <tdslite/detail/tds_span.hpp>
#include <tdslite/detail/tds_macrodef.hpp>
#include <gtest/gtest.h>

TEST(span, construct_from_fs_array) {
    tdslite::uint8_t buf [4];
    tdslite::span<tdslite::uint8_t> buf_span{buf};
    EXPECT_EQ(buf_span.data(), buf);
    EXPECT_EQ(buf_span.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span.begin(), buf);
    EXPECT_EQ(buf_span.end(), &buf [4]);
    EXPECT_TRUE(buf_span);
}

TEST(span, construct_from_buf_with_size) {
    tdslite::uint8_t buf [4];
    tdslite::span<tdslite::uint8_t> buf_span{buf, sizeof(buf)};
    EXPECT_EQ(buf_span.data(), buf);
    EXPECT_EQ(buf_span.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span.begin(), buf);
    EXPECT_EQ(buf_span.end(), &buf [4]);
    EXPECT_TRUE(buf_span);
}

TEST(span, construct_from_buf_with_begin_end) {
    tdslite::uint8_t buf [4];
    tdslite::span<tdslite::uint8_t> buf_span{buf, &buf [4]};
    EXPECT_EQ(buf_span.data(), buf);
    EXPECT_EQ(buf_span.size_bytes(), sizeof(buf));
    EXPECT_EQ(buf_span.begin(), buf);
    EXPECT_EQ(buf_span.end(), &buf [4]);
    EXPECT_TRUE(buf_span);
}

TEST(span, copy_construct) {
    tdslite::uint8_t buf [4];
    tdslite::span<tdslite::uint8_t> buf_span{buf};
    tdslite::span<tdslite::uint8_t> buf_span_cc{buf_span};
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
    tdslite::uint8_t buf [4];
    tdslite::span<tdslite::uint8_t> buf_span{buf};
    tdslite::span<tdslite::uint8_t> buf_span_cc = buf_span;
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
    tdslite::uint8_t buf [4];
    tdslite::span<tdslite::uint8_t> buf_span{buf};
    tdslite::span<tdslite::uint8_t> buf_span_mc{TDSLITE_MOVE(buf_span)};
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
    tdslite::uint8_t buf [4];
    tdslite::span<tdslite::uint8_t> buf_span{buf};
    tdslite::span<tdslite::uint8_t> buf_span_mc = TDSLITE_MOVE(buf_span);
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