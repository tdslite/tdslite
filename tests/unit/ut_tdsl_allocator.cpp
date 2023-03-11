/**
 * ____________________________________________________
 * unit tests for tdsl_allocator and tdsl_malloc_free
 *
 * @file   ut_tds_allocator.cpp
 * @author mkg <me@mustafagilor.com>
 * @date   01.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */
#ifndef NDEBUG
#define NDEBUG // to disable assertions
#endif
#include <tdslite/detail/tdsl_allocator.hpp>
#include <gtest/gtest.h>

#include <cstdlib>

class alloc_f : public ::testing::Test {};

// --------------------------------------------------------------------------------

TEST_F(alloc_f, default_alloc) {
    ASSERT_NE(tdsl::tdslite_malloc_free().a, nullptr);
    ASSERT_NE(tdsl::tdslite_malloc_free().f, nullptr);
    EXPECT_EQ(tdsl::tdslite_malloc_free().a, tdsl::tdsl_default_malloc);
    EXPECT_EQ(tdsl::tdslite_malloc_free().f, tdsl::tdsl_default_free);
}

// --------------------------------------------------------------------------------

TEST_F(alloc_f, set_alloc_invalid) {
    auto mf = tdsl::tdslite_malloc_free();
    tdsl::tdslite_malloc_free(std::malloc, nullptr);
    tdsl::tdslite_malloc_free(nullptr, std::free);
    EXPECT_EQ(tdsl::tdslite_malloc_free().a, mf.a);
    EXPECT_EQ(tdsl::tdslite_malloc_free().f, mf.f);
}

// --------------------------------------------------------------------------------

TEST_F(alloc_f, set_custom_alloc) {

    auto my_malloc = +[](size_t) -> void * {
        return nullptr;
    };

    auto my_free = +[](void *) {};

    tdsl::tdslite_malloc_free(my_malloc, my_free);
    EXPECT_EQ(tdsl::tdslite_malloc_free().a, my_malloc);
    EXPECT_EQ(tdsl::tdslite_malloc_free().f, my_free);
}

// --------------------------------------------------------------------------------

TEST_F(alloc_f, custom_alloc_free) {

    static int malloc_called_times = 0;
    static int free_called_times   = 0;

    auto my_malloc                 = +[](size_t) -> void * {
        malloc_called_times++;
        return nullptr;
    };

    auto my_free = +[](void *) {
        free_called_times++;
    };

    tdsl::tdslite_malloc_free(my_malloc, my_free);

    ASSERT_EQ(tdsl::tdslite_malloc_free().a, my_malloc);
    ASSERT_EQ(tdsl::tdslite_malloc_free().f, my_free);

    for (int i = 0; i < 500; i++) {
        auto alloc = tdsl::tds_allocator<int>::allocate(5);
        tdsl::tds_allocator<int>::deallocate(alloc, 5);
    }
    EXPECT_EQ(malloc_called_times, 500);
    EXPECT_EQ(free_called_times, 500);
}

// --------------------------------------------------------------------------------

TEST_F(alloc_f, create_destroy) {
    static int constructor_called_times = 0;
    static int destructor_called_times  = 0;
    static int malloc_called_times      = 0;
    static int free_called_times        = 0;

    struct my_type {

        my_type() {
            constructor_called_times++;
        }

        ~my_type() {
            destructor_called_times++;
        }
    };

    auto my_malloc = +[](size_t) -> void * {
        malloc_called_times++;
        static char buf [4096] = {};
        return buf;
    };

    auto my_free = +[](void *) {
        free_called_times++;
    };

    tdsl::tdslite_malloc_free(my_malloc, my_free);

    ASSERT_EQ(tdsl::tdslite_malloc_free().a, my_malloc);
    ASSERT_EQ(tdsl::tdslite_malloc_free().f, my_free);

    const int loop_cnt = 500;

    for (int i = 0; i < loop_cnt; i++) {
        auto alloc = tdsl::tds_allocator<my_type>::create();
        tdsl::tds_allocator<my_type>::destroy(alloc);
    }
    EXPECT_EQ(malloc_called_times, loop_cnt);
    EXPECT_EQ(free_called_times, loop_cnt);
    EXPECT_EQ(constructor_called_times, loop_cnt);
    EXPECT_EQ(destructor_called_times, loop_cnt);
}