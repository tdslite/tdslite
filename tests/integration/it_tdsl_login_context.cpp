/**
 * ____________________________________________________
 * TDS login context integration tests
 *
 * @file   it_tdsl_login_context.cpp
 * @author mkg <me@mustafagilor.com>
 * @date   20.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <tdslite/detail/tdsl_login_context.hpp>
#include <tdslite/detail/tdsl_command_context.hpp>
#include <tdslite-net/asio/tdsl_netimpl_asio.hpp>
#include <tdslite/util/tdsl_hex_dump.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <chrono>

// --------------------------------------------------------------------------------

/**
 * The type of the unit-under-test
 */
using uut_t     = tdsl::detail::login_context<tdsl::net::tdsl_netimpl_asio>;

using tds_ctx_t = uut_t::tds_context_type;

struct tds_login_ctx_it_fixture : public ::testing::Test {

    virtual void SetUp() override {
        ASSERT_EQ(0, tds_ctx.do_connect("mssql-2017", /*port=*/1433));
    }

    virtual void TearDown() override {}

    tds_ctx_t tds_ctx;
    uut_t login{tds_ctx};
};

// --------------------------------------------------------------------------------

TEST_F(tds_login_ctx_it_fixture, login) {
    uut_t::login_parameters params;
    params.server_name  = "mssql-2017";
    params.user_name    = "sa";
    params.password     = "2022-tds-lite-test!";
    params.client_name  = "tdslite integration test case";
    params.app_name     = "tdslite integration test";
    params.library_name = "tdslite";
    params.db_name      = "master";
    EXPECT_EQ(login.do_login(params), uut_t::e_login_status::success);
}

// --------------------------------------------------------------------------------

TEST_F(tds_login_ctx_it_fixture, login_invalid_uname) {
    uut_t::login_parameters params;
    params.server_name  = "mssql-2017";
    params.user_name    = "as";
    params.password     = "2022-tds-lite-test!";
    params.client_name  = "tdslite integration test case";
    params.app_name     = "tdslite integration test";
    params.library_name = "tdslite";
    params.db_name      = "master";
    EXPECT_EQ(login.do_login(params), uut_t::e_login_status::failure);
}

// --------------------------------------------------------------------------------

TEST_F(tds_login_ctx_it_fixture, login_invalid_pwd) {
    uut_t::login_parameters params;
    params.server_name  = "mssql-2017";
    params.user_name    = "sa";
    params.password     = "2022-tds-lite-test?";
    params.client_name  = "tdslite integration test case";
    params.app_name     = "tdslite integration test";
    params.library_name = "tdslite";
    params.db_name      = "master";
    EXPECT_EQ(login.do_login(params), uut_t::e_login_status::failure);
}

// --------------------------------------------------------------------------------

TEST_F(tds_login_ctx_it_fixture, login_invalid_db) {
    uut_t::login_parameters params;
    params.server_name  = "mssql-2017";
    params.user_name    = "sa";
    params.password     = "2022-tds-lite-test!";
    params.client_name  = "tdslite integration test case";
    params.app_name     = "tdslite integration test";
    params.library_name = "tdslite";
    params.db_name      = "masterofpuppets";
    EXPECT_EQ(login.do_login(params), uut_t::e_login_status::failure);
}
