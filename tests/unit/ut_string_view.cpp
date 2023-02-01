/**
 * ____________________________________________________
 * Unit tests for string_view and wstring_view types
 *
 * @file   ut_string_view.cpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   21.01.2023
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <tdslite/util/tdsl_string_view.hpp>
#include <gtest/gtest.h>

// --------------------------------------------------------------------------------

TEST(string_view, construct_default) {
    tdsl::string_view sv{};
    ASSERT_FALSE(sv);
    ASSERT_EQ(sv.size(), 0);
    ASSERT_EQ(sv.size_bytes(), 0);
    ASSERT_EQ(sv.data(), nullptr);
}

// --------------------------------------------------------------------------------

TEST(string_view, construct_from_nul_terminated_str) {
    const char buf [] = "this is a test";
    tdsl::string_view sv{buf};
    ASSERT_EQ(sv.size_bytes(), sv.size());
    ASSERT_EQ(sv.size(), 14);

    auto idx = 0;
    for (const auto & ch : sv) {
        ASSERT_EQ(ch, buf [idx++]);
    }
}

// --------------------------------------------------------------------------------

TEST(string_view, construct_from_non_null_terminated_str) {
    const char buf [] = {'t', 'h', 'i', 's', ' ', 'i', 's', ' ', 'a', ' ', 't', 'e', 's', 't'};
    tdsl::string_view sv{buf};
    ASSERT_EQ(sv.size_bytes(), sv.size());
    ASSERT_EQ(sv.size(), 14);

    auto idx = 0;
    for (const auto & ch : sv) {
        ASSERT_EQ(ch, buf [idx++]);
    }
}

// --------------------------------------------------------------------------------

TEST(wstring_view, construct_default) {
    tdsl::wstring_view sv{};
    ASSERT_FALSE(sv);
    ASSERT_EQ(sv.size(), 0);
    ASSERT_EQ(sv.size_bytes(), 0);
    ASSERT_EQ(sv.data(), nullptr);
}

// --------------------------------------------------------------------------------

TEST(wstring_view, construct_from_raw_bytes) {
    const tdsl::uint8_t buf [] = {'t', '\0', 'h', '\0', 'i', '\0', 's', '\0', '\0', '\0'};
    tdsl::wstring_view sv{buf};
    ASSERT_EQ(sv.size_bytes(), 8);
    ASSERT_EQ(sv.size(), 4);

    auto idx = 0;
    for (const auto & ch : sv) {
        ASSERT_EQ(ch, reinterpret_cast<const char16_t *>(buf) [idx++]);
    }
}

// --------------------------------------------------------------------------------

TEST(wstring_view, construct_from_raw_bytes_invalid) {
    const tdsl::uint8_t buf [] = {'t'};
    tdsl::wstring_view sv{buf};
    ASSERT_EQ(sv.size_bytes(), 0);
    ASSERT_EQ(sv.size(), 0);
}

// --------------------------------------------------------------------------------

TEST(wstring_view, construct_from_nul_terminated_u16_char_str) {
    const char16_t buf [] = u"this is a test";
    tdsl::wstring_view sv{buf};
    ASSERT_EQ(sv.size_bytes(), 28);
    ASSERT_EQ(sv.size(), 14);

    auto idx = 0;
    for (const auto & ch : sv) {
        ASSERT_EQ(ch, reinterpret_cast<const char16_t *>(buf) [idx++]);
    }
}

// --------------------------------------------------------------------------------

TEST(wstring_view, construct_from_non_nul_terminated_u16_char_str) {
    const char16_t buf [] = {u't', u'h', u'i', u's', u' ', u'i', u's',
                             u' ', u'a', u' ', u't', u'e', u's', u't'};
    tdsl::wstring_view sv{buf};
    ASSERT_EQ(sv.size_bytes(), 28);
    ASSERT_EQ(sv.size(), 14);

    auto idx = 0;
    for (const auto & ch : sv) {
        ASSERT_EQ(ch, reinterpret_cast<const char16_t *>(buf) [idx++]);
    }
}