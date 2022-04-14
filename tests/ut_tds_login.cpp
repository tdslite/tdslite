/**
 * _________________________________________________
 * Unit tests for tds login helper
 *
 * @file   ut_tds_login.cpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   14.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#include <tdslite/tds/login_context.hpp>
#include <gtest/gtest.h>

struct mock_network_impl {
    inline void do_write(tdslite::span<const tdslite::uint8_t> data) noexcept {
        (void) data;
    }

    inline void do_send(void) noexcept {}
};

TEST(test, test) {
    tdslite::tds::login_context<mock_network_impl> login;
    login.do_login("a", "b", "c", "d");
}