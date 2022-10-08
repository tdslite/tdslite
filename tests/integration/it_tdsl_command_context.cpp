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

#define TDSL_DEBUG_PRINT_ENABLED

#include <tdslite/detail/tdsl_login_context.hpp>
#include <tdslite/detail/tdsl_command_context.hpp>
#include <tdslite/net/asio/asio_network_impl.hpp>
#include <tdslite/util/tdsl_expected.hpp>
#include <tdslite/util/tdsl_hex_dump.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

// --------------------------------------------------------------------------------

static void default_row_callback(void *, const tdsl::tds_colmetadata_token & colmd,
                                 const tdsl::tdsl_row & row_data) {
    std::printf("colcnt %d\n", colmd.columns.size());
    std::printf("row with %d fields\n", row_data.size());
    for (tdsl::uint32_t i = 0; i < row_data.size(); i++) {
        std::printf("%d: ", i);
        tdsl::util::hexprint(row_data [i].data(), row_data [i].size());
    }
    std::printf("\n");
}

/**
 * The type of the unit-under-test
 */
using uut_t       = tdsl::detail::command_context<tdsl::net::asio_network_impl>;
using login_ctx_t = tdsl::detail::login_context<tdsl::net::asio_network_impl>;
using tds_ctx_t   = uut_t::tds_context_type;

/**
 * tds_command_context integration tests.
 *
 * NOTE: Prefix all table names with single `#` so
 * the table becomes a session-specific temporary table.
 * Otherwise, concurrent tests running in parallel can step
 * each other's feet.
 */
struct tds_command_ctx_it_fixture : public ::testing::Test {

    virtual void SetUp() override {
        login_ctx_t::login_parameters params;
        login_ctx_t login{tds_ctx};
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

    tds_ctx_t tds_ctx;
    uut_t command_ctx{tds_ctx};
};

TEST_F(tds_command_ctx_it_fixture, ct_int_int) {
    command_ctx.execute_query(
        tdsl::string_view{/*str=*/"CREATE TABLE #test_ct_int_int(q int,y int);"}); // callback
}

TEST_F(tds_command_ctx_it_fixture, cti_int_int) {
    command_ctx.execute_query(
        tdsl::string_view{/*str=*/"CREATE TABLE #test_cti_int_int(q int,y int);"}); // callback
    command_ctx.execute_query(
        tdsl::string_view{/*str=*/"INSERT INTO #test_cti_int_int VALUES(1,1);"}); // callback
}

TEST_F(tds_command_ctx_it_fixture, ctis_int_int) {
    // row callback
    // done callback?
    // should return 0 rows affected?
    EXPECT_EQ(0, command_ctx.execute_query(tdsl::string_view{
                     /*str=*/"CREATE TABLE #test_ctis_int_int(q int,y int);"})); // callback
    // should return 1 rows affected
    EXPECT_EQ(1, command_ctx.execute_query(tdsl::string_view{
                     /*str=*/"INSERT INTO #test_ctis_int_int VALUES(1,1);"})); // callback
    EXPECT_EQ(1, command_ctx.execute_query(tdsl::string_view{
                     /*str=*/"INSERT INTO #test_ctis_int_int VALUES(1,1);"})); // callback
    EXPECT_EQ(1, command_ctx.execute_query(tdsl::string_view{
                     /*str=*/"INSERT INTO #test_ctis_int_int VALUES(1,1);"})); // callback

    // should return 1 rows with 2 int fields.
    tdsl::uint32_t rows_affected = command_ctx.execute_query(
        tdsl::string_view{/*str=*/"SELECT q,y from #test_ctis_int_int;"}, nullptr,
        default_row_callback); // callback
    std::printf("rows affected %d\n", rows_affected);
}

TEST_F(tds_command_ctx_it_fixture, ctis_varcharn_real) {
    // row callback
    // done callback?

    // should return 0 rows affected?
    EXPECT_EQ(
        0,
        command_ctx.execute_query(tdsl::string_view{
            /*str=*/"CREATE TABLE #test_ctis_varcharn_real(q varchar(255),y real);"})); // callback
    // should return 1 rows affected
    EXPECT_EQ(1,
              command_ctx.execute_query(tdsl::string_view{
                  /*str=*/"INSERT INTO #test_ctis_varcharn_real VALUES('aaaa',0.5);"})); // callback

    tdsl::uint32_t rows_affected = command_ctx.execute_query(
        tdsl::string_view{/*str=*/"SELECT q,y from #test_ctis_varcharn_real;"}, nullptr,
        default_row_callback); // callback
    std::printf("rows affected %d\n", rows_affected);
}

TEST_F(tds_command_ctx_it_fixture, ctis_decimal_real) {
    // row callback
    // done callback?

    // should return 0 rows affected?
    EXPECT_EQ(0,
              command_ctx.execute_query(tdsl::string_view{
                  /*str=*/"CREATE TABLE #test_ctis_decimal_real(q decimal,y real);"})); // callback
    // should return 1 rows affected
    EXPECT_EQ(1, command_ctx.execute_query(tdsl::string_view{
                     /*str=*/"INSERT INTO #test_ctis_decimal_real VALUES(1,0.5);"})); // callback

    tdsl::uint32_t rows_affected = command_ctx.execute_query(
        tdsl::string_view{/*str=*/"SELECT q,y from #test_ctis_decimal_real;"}, nullptr,
        default_row_callback); // callback
    std::printf("rows affected %d\n", rows_affected);
}

TEST_F(tds_command_ctx_it_fixture, ctis_guid_varchar_int) {
    // row callback
    // done callback?

    // should return 0 rows affected?
    EXPECT_EQ(0, command_ctx.execute_query(tdsl::string_view{
                     /*str=*/"CREATE TABLE #test_ctis_guid_varchar_int(q "
                             "UNIQUEIDENTIFIER,y varchar(512),z int);"})); // callback
    // should return 1 rows affected
    EXPECT_EQ(1, command_ctx.execute_query(tdsl::string_view{
                     /*str=*/"INSERT INTO #test_ctis_guid_varchar_int VALUES(0x0, "
                             "'this is a test', 0);"})); // callback

    tdsl::uint32_t rows_affected = command_ctx.execute_query(
        tdsl::string_view{/*str=*/"SELECT q,y,z from #test_ctis_guid_varchar_int;"}, nullptr,
        default_row_callback); // callback
    std::printf("rows affected %d\n", rows_affected);
}

TEST_F(tds_command_ctx_it_fixture, ctis_10k_rows_multi_packet) {
    // row callback
    // done callback?

    static int callback_invoked = 0;

    // should return 0 rows affected?
    ASSERT_EQ(0, command_ctx.execute_query(tdsl::string_view{
                     /*str=*/"CREATE TABLE #test_ctis_10k_rows_multi_packet(q "
                             "UNIQUEIDENTIFIER,y varchar(512),z int);"})); //
                                                                           // callback

    for (int i = 0; i < 10000; i++) {
        ASSERT_EQ(1, command_ctx.execute_query(
                         tdsl::string_view{/*str=*/"INSERT INTO #test_ctis_10k_rows_multi_packet "
                                                   "VALUES(0x0, 'this is a test', 0);"})); //
        // callback
    }
    // should return 1 rows affected

    auto callback =
        +[](void *, const tdsl::tds_colmetadata_token & colmd, const tdsl::tdsl_row & row_data) {
            default_row_callback(nullptr, colmd, row_data);
            callback_invoked++;
        };

    {
        tdsl::uint32_t rows_affected = command_ctx.execute_query(
            tdsl::string_view{/*str=*/"SELECT q,y,z from #test_ctis_10k_rows_multi_packet;"},
            nullptr, callback); // callback
        std::printf("rows affected %d\n", rows_affected);
        EXPECT_EQ(10000, rows_affected);
        EXPECT_EQ(10000, callback_invoked);
    }
    {
        tdsl::uint32_t rows_affected = command_ctx.execute_query(
            tdsl::string_view{
                /*str=*/"SELECT TOP 500 q,y,z from #test_ctis_10k_rows_multi_packet ORDER BY z;"},
            nullptr, callback); // callback
        std::printf("rows affected %d\n", rows_affected);
        EXPECT_EQ(500, rows_affected);
        EXPECT_EQ(10500, callback_invoked);
    }
}

TEST_F(tds_command_ctx_it_fixture, ctis_guid_varchar_int_null) {

    // const char q[]= "DROP TABLE #test_ctis_guid_varchar_int_null;CREATE TABLE"
    //::testing::UnitTest::GetInstance()->current_test_case()->
    EXPECT_EQ(0, command_ctx.execute_query(tdsl::string_view{
                     /*str=*/"CREATE TABLE #test_ctis_guid_varchar_int_null(q "
                             "UNIQUEIDENTIFIER,y varchar(512),z int);"})); // callback
    // should return 1 rows affected
    EXPECT_EQ(1, command_ctx.execute_query(
                     tdsl::string_view{/*str=*/"INSERT INTO #test_ctis_guid_varchar_int_null "
                                               "VALUES(NULL, NULL, NULL);"})); // callback

    auto callback =
        +[](void *, const tdsl::tds_colmetadata_token & colmd, const tdsl::tdsl_row & row_data) {
            default_row_callback(nullptr, colmd, row_data);
            for (tdsl::uint32_t i = 0; i < row_data.size(); i++) {
                EXPECT_EQ(true, row_data [i].is_null());
            }
        };

    tdsl::uint32_t rows_affected = command_ctx.execute_query(
        tdsl::string_view{/*str=*/"SELECT q,y,z from #test_ctis_guid_varchar_int_null;"}, nullptr,
        callback); // callback
    std::printf("rows affected %d\n", rows_affected);
    EXPECT_EQ(rows_affected, 1);
}

TEST_F(tds_command_ctx_it_fixture, ctis_long_query_test) {

    // const char q[]= "DROP TABLE #test_ctis_guid_varchar_int_null;CREATE TABLE"
    //::testing::UnitTest::GetInstance()->current_test_case()->
    EXPECT_EQ(0, command_ctx.execute_query(tdsl::string_view{
                     /*str=*/"CREATE TABLE #test_ctis_guid_varchar_int_null(q "
                             "UNIQUEIDENTIFIER,y varchar(512),z int);"})); // callback
    std::string queries;
    for (int i = 0; i < 100; i++) {
        queries += "INSERT INTO #test_ctis_guid_varchar_int_null VALUES(NULL, NULL, NULL);";
    }

    // should return 1 rows affected, 100 times
    EXPECT_EQ(1, command_ctx.execute_query(tdsl::string_view{
                     queries.data(), static_cast<tdsl::uint32_t>(queries.size())})); // callback

    auto callback =
        +[](void *, const tdsl::tds_colmetadata_token & colmd, const tdsl::tdsl_row & row_data) {
            default_row_callback(nullptr, colmd, row_data);
            for (tdsl::uint32_t i = 0; i < row_data.size(); i++) {
                EXPECT_EQ(true, row_data [i].is_null());
            }
        };

    tdsl::uint32_t rows_affected = command_ctx.execute_query(
        tdsl::string_view{/*str=*/"SELECT q,y,z from #test_ctis_guid_varchar_int_null;"}, nullptr,
        callback); // callback
    std::printf("rows affected %d\n", rows_affected);
    EXPECT_EQ(rows_affected, 100);
}
