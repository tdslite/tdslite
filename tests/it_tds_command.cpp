/**
 * _________________________________________________
 *
 * @file   it_tds_command.cpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   23.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#include <tdslite/detail/tdsl_login_context.hpp>
#include <tdslite/detail/tdsl_command_context.hpp>
#include <tdslite/net/asio/asio_network_impl.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

// --------------------------------------------------------------------------------

/**
 * The type of the unit-under-test
 */
using uut_t       = tdsl::detail::command_context<tdsl::net::asio_network_impl>;
using login_ctx_t = tdsl::detail::login_context<tdsl::net::asio_network_impl>;
using tds_ctx_t   = uut_t::tds_context_type;

struct tds_command_ctx_it_fixture : public ::testing::Test {

    virtual void SetUp() override {

        params.server_name  = "mssql-2017";
        params.user_name    = "sa";
        params.password     = "2022-tds-lite-test!";
        params.client_name  = "tdslite integration test case";
        params.app_name     = "tdslite integration test";
        params.library_name = "tdslite";
        params.db_name      = "master";

        ASSERT_EQ(tds_ctx.do_connect("mssql-2017", /*port=*/1433), 0);
        ASSERT_EQ(login.do_login(params), login_ctx_t::e_login_status::success);
        ASSERT_TRUE(tds_ctx.is_authenticated());
    }

    virtual void TearDown() override {}
    tds_ctx_t tds_ctx;
    uut_t command_ctx{tds_ctx};
    login_ctx_t::login_parameters params;
    login_ctx_t login{tds_ctx};
};

TEST_F(tds_command_ctx_it_fixture, test) {
    command_ctx.execute_non_query(tdsl::string_view{/*str=*/"DROP TABLE test;CREATE TABLE test(q int,y int);"}); // callback
}