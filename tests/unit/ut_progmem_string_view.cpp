/**
 * ____________________________________________________
 * progmem_string_view class unit tests
 *
 * @file   ut_progmem_string_view.cpp
 * @author mkg <me@mustafagilor.com>
 * @date   04.12.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <cstring>

// mock:
#define PROGMEM
#define pgm_read_byte_near(x) *(x)

void * memcpy_P(void * __restrict dest, const void * __restrict src, size_t n) {
    return std::memcpy(dest, src, n);
}

#include <tdslite/util/tdsl_string_view.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

const char pmemstr [] = "Lorem ipsum dolor sit amet";

const char nntstr []  = {'L', 'o', 'r', 'e', 'm'};

// --------------------------------------------------------------------------------

TEST(progmem_string_view, default_construct) {
    tdsl::progmem_string_view psv{pmemstr};
    EXPECT_EQ(psv.size_bytes(), sizeof(pmemstr) - 1);
}

// --------------------------------------------------------------------------------

TEST(progmem_string_view, default_construct_nntstr) {
    tdsl::progmem_string_view psv{nntstr};
    EXPECT_EQ(psv.size_bytes(), sizeof(nntstr));
}

// --------------------------------------------------------------------------------

TEST(progmem_string_view, iterate_range_loop) {
    int idx = 0;
    for (auto ch : tdsl::progmem_string_view{pmemstr}) {
        ASSERT_EQ(ch, pmemstr [idx++]);
    }
}

// --------------------------------------------------------------------------------

TEST(progmem_string_view, iterate_post_increment) {
    int idx = 0;
    tdsl::progmem_string_view psv{pmemstr};
    for (auto itr = psv.begin(); itr != psv.end(); itr++) {
        ASSERT_EQ(*itr, pmemstr [idx++]);
    }
}

// --------------------------------------------------------------------------------

TEST(progmem_string_view, iterate_pre_increment) {
    int idx = 0;
    tdsl::progmem_string_view psv{pmemstr};
    for (auto itr = psv.begin(); itr != psv.end(); ++itr) {
        ASSERT_EQ(*itr, pmemstr [idx++]);
    }
}

// --------------------------------------------------------------------------------

TEST(progmem_string_view, iterate_array_subscript) {
    tdsl::progmem_string_view psv{pmemstr};
    for (tdsl::size_t i = 0; i < psv.size_bytes(); i++) {
        ASSERT_EQ(psv [i], pmemstr [i]);
    }
}