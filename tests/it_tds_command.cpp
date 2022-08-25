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
#include <tdslite/util/tdsl_expected.hpp>

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
    // row callback
    // done callback?
    command_ctx.execute_query(tdsl::string_view{/*str=*/"DROP TABLE test;CREATE TABLE test(q int,y int);"}); // callback
}

TEST_F(tds_command_ctx_it_fixture, test2) {
    // row callback
    // done callback?
    command_ctx.execute_query(tdsl::string_view{/*str=*/"DROP TABLE test;CREATE TABLE test(q int,y int);"}); // callback
    command_ctx.execute_query(tdsl::string_view{/*str=*/"INSERT INTO test VALUES(1,1);"});                   // callback
}

TEST_F(tds_command_ctx_it_fixture, test3) {
    // row callback
    // done callback?

    // should return 0 rows affected?
    EXPECT_EQ(0, command_ctx.execute_query(tdsl::string_view{/*str=*/"DROP TABLE test;CREATE TABLE test(q int,y int);"})); // callback
    // should return 1 rows affected
    EXPECT_EQ(1, command_ctx.execute_query(tdsl::string_view{/*str=*/"INSERT INTO test VALUES(1,1);"})); // callback
    EXPECT_EQ(1, command_ctx.execute_query(tdsl::string_view{/*str=*/"INSERT INTO test VALUES(1,1);"})); // callback
    EXPECT_EQ(1, command_ctx.execute_query(tdsl::string_view{/*str=*/"INSERT INTO test VALUES(1,1);"})); // callback

    auto callback = +[](void *, const tdsl::tds_colmetadata_token & colmd, const tdsl::tdsl_row & row_data) {
        std::printf("colcnt %d\n", colmd.column_count);
        std::printf("row with %d fields\n", row_data.fields.size());
    };
    // should return 1 rows with 2 int fields.
    tdsl::uint32_t rows_affected =
        command_ctx.execute_query(tdsl::string_view{/*str=*/"SELECT q,y from test;"}, nullptr, callback); // callback
    std::printf("rows affected %d\n", rows_affected);
}