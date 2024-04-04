/**
 * ____________________________________________________
 * tdsl_driver integration tests
 *
 * @file   it_tdsl_driver.cpp
 * @author mkg <hello@mkg.dev>
 * @date   04.04.2024
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <tdslite/detail/tdsl_driver.hpp>
#include <tdslite-net/asio/tdsl_netimpl_asio.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using uut_t       = tdsl::detail::tdsl_driver<tdsl::net::tdsl_netimpl_asio>;
using tds_ctx_t   = uut_t::tds_context_type;
using login_ctx_t = tdsl::detail::login_context<tdsl::net::tdsl_netimpl_asio>;

/**
 * Credentials for internal mssql 2022
 *
 * @return const login_ctx_t::login_parameters&
 */
static inline auto mssql_2022_creds() -> const uut_t::connection_parameters & {
    static uut_t::connection_parameters params = [] {
        uut_t::connection_parameters p;
        p.server_name  = "mssql-2022";
        p.user_name    = "sa";
        p.password     = "2022-tds-lite-test!";
        p.client_name  = "tdslite integration test case";
        p.app_name     = "tdslite integration test";
        p.library_name = "tdslite";
        p.db_name      = "master";
        return p;
    }();

    return params;
}

/**
 * tds_command_context integration tests.
 *
 * NOTE: Prefix all table names with single `#` so
 * the table becomes a session-specific temporary table.
 * Otherwise, concurrent tests running in parallel can step
 * each other's feet.
 */
struct tds_driver_it_fixture : public ::testing::Test {

    virtual void SetUp() override {
        auto r = uut.connect(mssql_2022_creds());
        ASSERT_EQ(r, uut_t::e_driver_error_code::success);
    }

public:
    uut_t uut{};
};

// --------------------------------------------------------------------------------

TEST_F(tds_driver_it_fixture, execute_query_char16_str) {
    auto r1 =
        uut.execute_query(u"CREATE TABLE #test_rpc(x bit, a tinyint, b smallint, c int, d bigint)");

    ASSERT_TRUE(r1);
    ASSERT_FALSE(r1.status.count_valid());
    ASSERT_FALSE(r1.status.attn());
    ASSERT_FALSE(r1.status.error());
    ASSERT_FALSE(r1.status.in_xact());
    ASSERT_FALSE(r1.status.more());
    ASSERT_FALSE(r1.status.srverror());
    ASSERT_EQ(0, r1.affected_rows);
    auto r2 = uut.execute_query(u"INSERT INTO #test_rpc VALUES(1, 255, 32767, 2100000000, 42)");

    ASSERT_TRUE(r2);
    ASSERT_TRUE(r2.status.count_valid());
    ASSERT_FALSE(r2.status.attn());
    ASSERT_FALSE(r2.status.error());
    ASSERT_FALSE(r2.status.in_xact());
    ASSERT_FALSE(r2.status.more());
    ASSERT_FALSE(r2.status.srverror());
    ASSERT_EQ(1, r2.affected_rows);
}
