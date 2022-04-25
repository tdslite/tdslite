/**
 * _________________________________________________
 * TDS login context integration tests
 *
 * @file   it_tds_login.cpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   20.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#include <tdslite/detail/tdsl_login_context.hpp>
#include <tdslite/net/asio/asio_network_impl.hpp>
#include <tdslite/util/tdsl_hex_dump.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <chrono>

// --------------------------------------------------------------------------------

/**
 * The type of the unit-under-test
 */
using uut_t     = tdsl::detail::login_context<tdsl::net::asio_network_impl>;

using tds_ctx_t = uut_t::tds_context_type;

struct tds_login_ctx_it_fixture : public ::testing::Test {

    virtual void SetUp() override {
        tds_ctx.do_connect("mssql-2017", /*port=*/1433);
    }

    virtual void TearDown() override {}
    tds_ctx_t tds_ctx;
    uut_t login{tds_ctx};
};

TEST_F(tds_login_ctx_it_fixture, login) {
    EXPECT_TRUE(tds_ctx.recv_buffer.size());
    std::this_thread::sleep_for(std::chrono::seconds{5});
    uut_t::login_parameters params;
    params.server_name  = "mssql-2017";
    params.user_name    = "sa";
    params.password     = "2022-tds-lite-test!";
    params.client_name  = "tdslite integration test case";
    params.app_name     = "tdslite integration test";
    params.library_name = "tdslite";
    params.db_name      = "master";
    login.do_login(
        params, +[](uut_t::e_login_status) {});
    std::this_thread::sleep_for(std::chrono::seconds{5});
}
