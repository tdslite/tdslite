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

#include <chrono>
#include <tdslite/tds/tds_login_context.hpp>
#include <tdslite/mock/asio_network_impl.hpp>
#include <tdslite/util/hex_dump.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>

// --------------------------------------------------------------------------------

/**
 * The type of the unit-under-test
 */
using uut_t = tdslite::tds::login_context<tdslite::mock::asio_network_impl>;

struct tds_login_ctx_it_fixture : public ::testing::Test {

    virtual void SetUp() override {
        login.do_connect("mssql-2017", /*port=*/1433);
    }

    virtual void TearDown() override {}

    uut_t login;
};

TEST_F(tds_login_ctx_it_fixture, login) {
    EXPECT_TRUE(login.recv_buffer.size());
    std::this_thread::sleep_for(std::chrono::seconds{5});
    uut_t::login_parameters params;
    params.server_name  = "mssql-2017";
    params.user_name    = "sa";
    params.password     = "2022-tds-lite-test!";
    params.client_name  = "tdslite integration test case";
    params.app_name     = "tdslite integration test";
    params.library_name = "tdslite";
    params.db_name      = "master";
    login.do_login(params);
    std::this_thread::sleep_for(std::chrono::seconds{5});
}
