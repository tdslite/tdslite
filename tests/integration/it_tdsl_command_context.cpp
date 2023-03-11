/**
 * ____________________________________________________
 * command_context integration tests
 *
 * @file   it_tdsl_command_context.cpp
 * @author mkg <me@mustafagilor.com>
 * @date   23.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

// #define TDSL_DEBUG_PRINT_ENABLED

#include <tdslite/detail/tdsl_login_context.hpp>
#include <tdslite/detail/tdsl_command_context.hpp>
#include <tdslite/net/asio/tdsl_netimpl_asio.hpp>
#include <tdslite/util/tdsl_expected.hpp>
#include <tdslite/util/tdsl_hex_dump.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

/**
 * The type of the unit-under-test
 */
using uut_t       = tdsl::detail::command_context<tdsl::net::tdsl_netimpl_asio>;
using login_ctx_t = tdsl::detail::login_context<tdsl::net::tdsl_netimpl_asio>;
using tds_ctx_t   = uut_t::tds_context_type;

// --------------------------------------------------------------------------------

static void default_row_callback(void *, uut_t::column_metadata_cref colmd,
                                 uut_t::row_cref row_data) {
#ifdef TDSL_TEST_VERBOSE_OUTPUT
    std::printf("colcnt %d\n", colmd.columns.size());
    std::printf("row with %d fields\n", row_data.size());
    for (tdsl::uint32_t i = 0; i < row_data.size(); i++) {
        std::printf("%d: ", i);
        tdsl::util::hexprint(row_data [i].data(), row_data [i].size());
    }
    std::printf("\n");
#else
    (void) colmd;
    (void) row_data;
#endif
}

/**
 * Credentials for internal mssql 2017
 *
 * @return const login_ctx_t::login_parameters&
 */
static inline auto mssql_2017_creds() -> const login_ctx_t::login_parameters & {
    static login_ctx_t::login_parameters params = [] {
        login_ctx_t::login_parameters p;
        p.server_name  = "mssql-2017";
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
struct tds_command_ctx_it_fixture : public ::testing::Test {

    virtual void SetUp() override {
        login_ctx_t login{tds_ctx};
        const auto & params = mssql_2017_creds();
        ASSERT_EQ(tds_ctx.do_connect(params.server_name, /*port=*/1433), 0);
        ASSERT_EQ(login.do_login(params), login_ctx_t::e_login_status::success);
        ASSERT_TRUE(tds_ctx.is_authenticated());
    }

    struct owning_string_view {
        inline operator tdsl::string_view() const noexcept {
            return tdsl::string_view{v.data(), static_cast<tdsl::uint32_t>(v.size())};
        }

        std::string v;
    };

    template <typename... Args>
    static inline owning_string_view cq(Args... args) noexcept {
        std::string result{"CREATE TABLE #"};
        result +=
            std::string(::testing::UnitTest::GetInstance()->current_test_info()->name()) + " (";
        int unpack []{0, (result += (std::string(args) + std::string(",")), 0)...};
        (void) unpack;
        result.pop_back(); // remove last excess comma
        result += ");";
        return {result};
    }

    template <typename... Args>
    static inline owning_string_view iq(Args... args) noexcept {
        std::string result{"INSERT INTO #"};
        result += std::string(::testing::UnitTest::GetInstance()->current_test_info()->name()) +
                  " VALUES(";
        int unpack []{0, (result += (std::string(args) + std::string(",")), 0)...};
        (void) unpack;
        result.pop_back(); // remove last excess comma
        result += ");";
        return {result};
    }

    // template <typename... Args>
    static inline owning_string_view sq(const char * s = "*", const char * w = nullptr,
                                        const char * o = nullptr) noexcept {
        std::string result{"SELECT "};
        result += s;
        result += " FROM #" +
                  std::string(::testing::UnitTest::GetInstance()->current_test_info()->name());
        if (w) {
            result += " WHERE " + std::string(w);
        }
        if (o) {
            result += " ORDER BY " + std::string(o);
        }
        result += ";";
        return {result};
    }

    template <tdsl::uint32_t Times = 1>
    inline tdsl::uint64_t exec(const owning_string_view & query,
                               uut_t::row_callback_fn_t rcb = default_row_callback,
                               void * uptr                  = nullptr) noexcept {
        tdsl::uint64_t total_rows_affected = 0;
        for (tdsl::uint32_t i = 0; i < Times; i++) {

            auto result =
                command_ctx.execute_query(static_cast<tdsl::string_view>(query), rcb, uptr);

            total_rows_affected += result.affected_rows;
        }
        return total_rows_affected;
    }

    template <tdsl::uint32_t Times = 1>
    inline tdsl::uint64_t
    exec_as_one(const owning_string_view & query, void * uptr = nullptr,
                uut_t::row_callback_fn_t rcb = default_row_callback) noexcept {
        std::string q;
        for (tdsl::uint32_t i = 0; i < Times; i++) {
            q += query.v;
        }

        TDSL_DEBUG_PRINTLN("exec_as_one--query length %lu", q.length());

        return command_ctx
            .execute_query(tdsl::string_view{q.data(), static_cast<tdsl::uint32_t>(q.size())}, rcb,
                           uptr)
            .affected_rows;
    }

    tds_ctx_t tds_ctx;
    uut_t command_ctx{tds_ctx};
    tdsl::size_t validator_called_times = {0};

    static inline void validator_called(void * v) noexcept {
        *reinterpret_cast<decltype(validator_called_times) *>(v) =
            *reinterpret_cast<decltype(validator_called_times) *>(v) + 1;
    }
};

TEST_F(tds_command_ctx_it_fixture, ct_int_int) {
    ASSERT_EQ(0, exec(cq("q int", "y int")));
}

TEST_F(tds_command_ctx_it_fixture, cti_int_int) {
    ASSERT_EQ(0, exec(cq("q int", "y int")));
    ASSERT_EQ(1, exec(iq("1", "1")));
}

TEST_F(tds_command_ctx_it_fixture, ctis_int_int) {
    ASSERT_EQ(0, exec(cq("q int", "y int")));
    ASSERT_EQ(3, exec<3>(iq("1", "1")));
}

// data_fill

TEST_F(tds_command_ctx_it_fixture, ctis_varcharn_real) {
    ASSERT_EQ(0, exec(cq("q varchar(255)", "y real")));
    ASSERT_EQ(5, exec<5>(iq("'aaaa'", "0.5")));
    ASSERT_EQ(5, exec(sq()));
}

TEST_F(tds_command_ctx_it_fixture, ctis_decimal_real) {
    ASSERT_EQ(0, exec(cq("q decimal", "y real")));
    ASSERT_EQ(3, exec<3>(iq("1", "0.5")));
    ASSERT_EQ(3, exec(sq()));
}

TEST_F(tds_command_ctx_it_fixture, ctis_guid_varchar_int) {
    ASSERT_EQ(0, exec(cq("q UNIQUEIDENTIFIER", "y varchar(512)", "z int")));
    ASSERT_EQ(3, exec<3>(iq("0x0", "'this is a test'", "0")));
    ASSERT_EQ(3, exec(sq()));
}

TEST_F(tds_command_ctx_it_fixture, ctis_10k_rows_multi_packet) {
    static int callback_invoked = 0;

    auto callback = +[](void *, uut_t::column_metadata_cref colmd, uut_t::row_cref row_data) {
        default_row_callback(nullptr, colmd, row_data);
        callback_invoked++;
    };
    ASSERT_EQ(0, exec(cq("q UNIQUEIDENTIFIER", "y varchar(512)", "z int")));
    ASSERT_EQ(10000, exec<10000>(iq("0x0", "'this is a test'", "0")));
    ASSERT_EQ(10000, exec(sq(), callback));
    ASSERT_EQ(callback_invoked, 10000);
    ASSERT_EQ(500, exec(sq("TOP 500 q,y,z", nullptr, "z"), callback));
    ASSERT_EQ(callback_invoked, 10500);
}

TEST_F(tds_command_ctx_it_fixture, ctis_guid_varchar_int_null) {
    ASSERT_EQ(0, exec(cq("q UNIQUEIDENTIFIER", "y varchar(512)", "z int")));
    ASSERT_EQ(3, exec<3>(iq("NULL", "NULL", "NULL")));
    ASSERT_EQ(3,
              exec(
                  sq(), +[](void *, uut_t::column_metadata_cref colmd, uut_t::row_cref row_data) {
                      default_row_callback(nullptr, colmd, row_data);
                      for (tdsl::uint32_t i = 0; i < row_data.size(); i++) {
                          EXPECT_EQ(true, row_data [i].is_null());
                      }
                  }));
}

TEST_F(tds_command_ctx_it_fixture, ctis_long_query_test) {
    ASSERT_EQ(0, exec(cq("q UNIQUEIDENTIFIER", "y varchar(512)", "z int")));
    ASSERT_EQ(1, exec_as_one<100>(iq("NULL", "NULL", "NULL")));
    ASSERT_EQ(100,
              exec(
                  sq(), +[](void *, uut_t::column_metadata_cref colmd, uut_t::row_cref row_data) {
                      default_row_callback(nullptr, colmd, row_data);
                      for (tdsl::uint32_t i = 0; i < row_data.size(); i++) {
                          EXPECT_EQ(true, row_data [i].is_null());
                      }
                  }));
}

// ------------------------------
// Exact numerics
// numeric,   ,decimal
// smallmoney, , money

// --------------------------------------------------------------------------------
TEST_F(tds_command_ctx_it_fixture, exact_numerics_bit) {
    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 2);
        ASSERT_EQ(r.size(), 2);
        EXPECT_EQ(r [0].as<bool>(), true);
        EXPECT_EQ(r [1].as<bool>(), false);
    };
    ASSERT_EQ(0, exec(cq("q bit", "y bit")));
    ASSERT_EQ(1, exec(iq("'true'", "'false'")));
    ASSERT_EQ(1, exec(sq(), validator, &validator_called_times));
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------
TEST_F(tds_command_ctx_it_fixture, exact_numerics_tinyint) {
    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 2);
        ASSERT_EQ(r.size(), 2);
        EXPECT_EQ(r [0].as<tdsl::uint8_t>(), tdsl::numeric_limits::min_value<tdsl::uint8_t>());
        EXPECT_EQ(r [1].as<tdsl::uint8_t>(), tdsl::numeric_limits::max_value<tdsl::uint8_t>());
    };
    ASSERT_EQ(0, exec(cq("q tinyint", "y tinyint")));
    ASSERT_EQ(1, exec(iq("0", "255")));
    ASSERT_EQ(1, exec(sq(), validator, &validator_called_times));
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------
TEST_F(tds_command_ctx_it_fixture, exact_numerics_smallint) {
    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 2);
        ASSERT_EQ(r.size(), 2);
        EXPECT_EQ(r [0].as<tdsl::int16_t>(), tdsl::numeric_limits::min_value<tdsl::int16_t>());
        EXPECT_EQ(r [1].as<tdsl::int16_t>(), tdsl::numeric_limits::max_value<tdsl::int16_t>());
    };
    ASSERT_EQ(0, exec(cq("q smallint", "y smallint")));
    ASSERT_EQ(1, exec(iq("-32768", "32767")));
    ASSERT_EQ(1, exec(sq(), validator, &validator_called_times));
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------
TEST_F(tds_command_ctx_it_fixture, exact_numerics_int) {
    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 2);
        ASSERT_EQ(r.size(), 2);
        EXPECT_EQ(r [0].as<tdsl::int32_t>(), tdsl::numeric_limits::min_value<tdsl::int32_t>());
        EXPECT_EQ(r [1].as<tdsl::int32_t>(), tdsl::numeric_limits::max_value<tdsl::int32_t>());
    };
    ASSERT_EQ(0, exec(cq("q int", "y int")));
    ASSERT_EQ(1, exec(iq("-2147483648", "2147483647")));
    ASSERT_EQ(1, exec(sq(), validator, &validator_called_times));
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------
TEST_F(tds_command_ctx_it_fixture, exact_numerics_bigint) {
    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 2);
        ASSERT_EQ(r.size(), 2);
        EXPECT_EQ(r [0].as<tdsl::int64_t>(), tdsl::numeric_limits::min_value<tdsl::int64_t>());
        EXPECT_EQ(r [1].as<tdsl::int64_t>(), tdsl::numeric_limits::max_value<tdsl::int64_t>());
    };
    ASSERT_EQ(0, exec(cq("q bigint", "y bigint")));
    ASSERT_EQ(1, exec(iq("-9223372036854775808", "9223372036854775807")));
    ASSERT_EQ(1, exec(sq(), validator, &validator_called_times));
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------
TEST_F(tds_command_ctx_it_fixture, exact_numerics_numeric) {
    /**
    * Precision	Storage bytes
           1 - 9	5
           10-19	9
           20-28	13
           29-38	17
    *
    */

    struct constraint {
        struct ps {
            tdsl::uint8_t precision;
            tdsl::uint8_t scale;
        } ps_v [2]              = {};

        const char * values [2] = {};
    } test_datas [5];

    test_datas [0].ps_v [0].precision = 1;
    test_datas [0].ps_v [0].scale     = 0;
    test_datas [0].values [0]         = "1.0";
    test_datas [0].ps_v [1].precision = 1;
    test_datas [0].ps_v [1].scale     = 1;
    test_datas [0].values [1]         = "0.1";

    test_datas [1].ps_v [0].precision = 2;
    test_datas [1].ps_v [0].scale     = 1;
    test_datas [1].values [0]         = "1.1";
    test_datas [1].ps_v [1].precision = 2;
    test_datas [1].ps_v [1].scale     = 2;
    test_datas [1].values [1]         = "0.12";

    test_datas [2].ps_v [0].precision = 10;
    test_datas [2].ps_v [0].scale     = 1;
    test_datas [2].values [0]         = "999999999.9";
    test_datas [2].ps_v [1].precision = 19;
    test_datas [2].ps_v [1].scale     = 2;
    test_datas [2].values [1]         = "12345678901234567.12";

    test_datas [3].ps_v [0].precision = 20;
    test_datas [3].ps_v [0].scale     = 10;
    test_datas [3].values [0]         = "9999999999.9999999999";
    test_datas [3].ps_v [1].precision = 28;
    test_datas [3].ps_v [1].scale     = 14;
    test_datas [3].values [1]         = "99999999999999.99999999999999";

    test_datas [4].ps_v [0].precision = 29;
    test_datas [4].ps_v [0].scale     = 14;
    test_datas [4].values [0]         = "999999999999999.99999999999999";
    test_datas [4].ps_v [1].precision = 38;
    test_datas [4].ps_v [1].scale     = 10;
    test_datas [4].values [1]         = "9999999999999999999999999999.9999999999";

    for (tdsl::uint32_t i = 0; i < sizeof(test_datas) / sizeof(constraint); i++) {
        auto & current_constraint = test_datas [i];

        auto validator = +[](void * uptr, uut_t::column_metadata_cref c, uut_t::row_cref r) {
            auto precision_size_validator = [](tdsl::uint32_t p, tdsl::uint32_t l) {
                if (p <= 9) {
                    EXPECT_EQ(l, 5);
                }
                else if (p >= 10 && p <= 19) {
                    EXPECT_EQ(l, 9);
                }
                else if (p >= 20 && p <= 28) {
                    EXPECT_EQ(l, 13);
                }
                else if (p >= 29 && p <= 38) {
                    EXPECT_EQ(l, 17);
                }
                else {
                    ASSERT_TRUE(false);
                }
            };
            const constraint & current_constraint = *reinterpret_cast<const constraint *>(uptr);
            ASSERT_EQ(c.columns.size(), 2);
            ASSERT_EQ(r.size(), 2);
            ASSERT_EQ(c.columns [0].typeprops.ps.precision, current_constraint.ps_v [0].precision);
            ASSERT_EQ(c.columns [0].typeprops.ps.scale, current_constraint.ps_v [0].scale);
            ASSERT_EQ(c.columns [1].typeprops.ps.precision, current_constraint.ps_v [1].precision);
            ASSERT_EQ(c.columns [1].typeprops.ps.scale, current_constraint.ps_v [1].scale);
            precision_size_validator(current_constraint.ps_v [0].precision, r [0].size_bytes());
            precision_size_validator(current_constraint.ps_v [1].precision, r [1].size_bytes());

            // TODO: Validate field values?
        };

        owning_string_view c1{"q numeric(" + std::to_string(current_constraint.ps_v [0].precision) +
                              "," + std::to_string(current_constraint.ps_v [0].scale) + ")"};

        owning_string_view c2{"y numeric(" + std::to_string(current_constraint.ps_v [1].precision) +
                              "," + std::to_string(current_constraint.ps_v [1].scale) + ")"};

        exec({"DROP TABLE #exact_numerics_numeric;"});
        ASSERT_EQ(0, exec(cq(c1.v.c_str(), c2.v.c_str())));
        ASSERT_EQ(1, exec(iq(current_constraint.values [0], current_constraint.values [1])));
        ASSERT_EQ(1, exec(sq(), validator, reinterpret_cast<void *>(&current_constraint)));
    }
}

// --------------------------------------------------------------------------------
TEST_F(tds_command_ctx_it_fixture, exact_numerics_smallmoney) {
    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 2);
        ASSERT_EQ(r.size(), 2);
        ASSERT_EQ(r [0].size_bytes(), 4);
        ASSERT_EQ(r [1].size_bytes(), 4);

        // prec-scale 10/4
        // FIXME: Use decimal after implementing it.
        EXPECT_EQ(r [0].as<tdsl::int32_t>(), tdsl::numeric_limits::min_value<tdsl::int32_t>());
        EXPECT_EQ(r [1].as<tdsl::int32_t>(), tdsl::numeric_limits::max_value<tdsl::int32_t>());
    };
    // smallmoney	- 214,748.3648 to 214,748.3647
    ASSERT_EQ(0, exec(cq("q smallmoney", "y smallmoney")));
    ASSERT_EQ(1, exec(iq("-214748.3648", "214748.3647")));
    ASSERT_EQ(1, exec(sq(), validator, &validator_called_times));
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------
TEST_F(tds_command_ctx_it_fixture, exact_numerics_money) {

    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 2);
        ASSERT_EQ(r.size(), 2);
        ASSERT_EQ(r [0].size_bytes(), 8);
        ASSERT_EQ(r [1].size_bytes(), 8);

        EXPECT_EQ(r [0].as<tdsl::sqltypes::sql_money>().raw(),
                  tdsl::numeric_limits::min_value<tdsl::int64_t>());
        EXPECT_EQ(r [1].as<tdsl::sqltypes::sql_money>().raw(),
                  tdsl::numeric_limits::max_value<tdsl::int64_t>());

        EXPECT_EQ(r [0].as<tdsl::sqltypes::sql_money>(), -922337203685477.5808);
        EXPECT_EQ(r [1].as<tdsl::sqltypes::sql_money>(), 922337203685477.5807);
    };
    // smallmoney	- 214,748.3648 to 214,748.3647
    ASSERT_EQ(0, exec(cq("q money", "y money")));
    ASSERT_EQ(1, exec(iq("-922337203685477.5808", "922337203685477.5807")));
    ASSERT_EQ(1, exec(sq(), validator, &validator_called_times));
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------
TEST_F(tds_command_ctx_it_fixture, exact_numerics_real) {
    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 2);
        ASSERT_EQ(r.size(), 2);
        EXPECT_EQ(r [0].as<float>(), float{1.17549e-038f});
        EXPECT_EQ(r [1].as<float>(), float{3.40282e+038f});
    };
    ASSERT_EQ(0, exec(cq("q real", "y real")));
    ASSERT_EQ(1, exec(iq("1.17549e-038", "3.40282e+038")));
    ASSERT_EQ(1, exec(sq(), validator, &validator_called_times));
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------

TEST_F(tds_command_ctx_it_fixture, test_rpc_inttype) {
    tdsl::detail::sql_parameter_bit px{};
    tdsl::detail::sql_parameter_tinyint p1{};
    tdsl::detail::sql_parameter_smallint p2{};
    tdsl::detail::sql_parameter_int p3{};
    tdsl::detail::sql_parameter_bigint p4{};

    // The binding order determines the variable name, e.g. bound parameter 0 is p0
    // and bound parameter 1 is p1 and so on.
    tdsl::detail::sql_parameter_binding params [] = {px, p1, p2, p3, p4};

    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 5);
        ASSERT_EQ(r.size(), 5);
        ASSERT_EQ(r [0].size_bytes(), 1);
        ASSERT_EQ(r [1].size_bytes(), 1);
        ASSERT_EQ(r [2].size_bytes(), 2);
        ASSERT_EQ(r [3].size_bytes(), 4);
        ASSERT_EQ(r [4].size_bytes(), 8);

        EXPECT_EQ(r [0].as<tdsl::sqltypes::s_bit>(), true);
        EXPECT_EQ(r [1].as<tdsl::sqltypes::s_tinyint>(), 255);
        EXPECT_EQ(r [2].as<tdsl::sqltypes::s_smallint>(), 32767);
        EXPECT_EQ(r [3].as<tdsl::sqltypes::s_int>(), 2100000000);
        EXPECT_EQ(r [4].as<tdsl::sqltypes::s_bigint>(), 42);
    };

    // Create the table
    auto r1 = command_ctx.execute_query(
        tdsl::string_view{"CREATE TABLE #test_rpc(x bit, a tinyint, b smallint, c int, d bigint)"});

    ASSERT_TRUE(r1);
    ASSERT_FALSE(r1.status.count_valid());
    ASSERT_FALSE(r1.status.attn());
    ASSERT_FALSE(r1.status.error());
    ASSERT_FALSE(r1.status.in_xact());
    ASSERT_FALSE(r1.status.more());
    ASSERT_FALSE(r1.status.srverror());
    ASSERT_EQ(0, r1.affected_rows);

    // Fill some data
    auto r2 = command_ctx.execute_query(
        tdsl::string_view{"INSERT INTO #test_rpc VALUES(1, 255, 32767, 2100000000, 42)"});

    ASSERT_TRUE(r2);
    ASSERT_TRUE(r2.status.count_valid());
    ASSERT_FALSE(r2.status.attn());
    ASSERT_FALSE(r2.status.error());
    ASSERT_FALSE(r2.status.in_xact());
    ASSERT_FALSE(r2.status.more());
    ASSERT_FALSE(r2.status.srverror());
    ASSERT_EQ(1, r2.affected_rows);

    command_ctx.execute_rpc(
        tdsl::string_view{
            "SELECT * FROM #test_rpc WHERE x=@p0 AND a=@p1 AND b=@p2 AND c=@p3 and d=@p4"},
        params, tdsl::detail::e_rpc_mode::executesql, validator, &validator_called_times);

    // Response should not contain a result set.

    // Update the parameters and execute the query again
    px = true, p1 = 255, p2 = 32767, p3 = 2100000000, p4 = 42;

    command_ctx.execute_rpc(
        tdsl::string_view{
            "SELECT * FROM #test_rpc WHERE x=@p0 AND a=@p1 AND b=@p2 AND c=@p3 and d=@p4"},
        params, tdsl::detail::e_rpc_mode::executesql, validator, &validator_called_times);
    ASSERT_EQ(validator_called_times, 1);
}

// --------------------------------------------------------------------------------

TEST_F(tds_command_ctx_it_fixture, test_rpc_varchar) {
    tdsl::detail::sql_parameter_varchar p1{};
    p1                                            = tdsl::string_view{"abc"};

    // Text types does not support variable update and must be rebound
    // to parameter binding again.

    // The binding order determines the variable name, e.g. bound parameter 0 is p0
    // and bound parameter 1 is p1 and so on.
    tdsl::detail::sql_parameter_binding params [] = {p1};

    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 1);
        ASSERT_EQ(r.size(), 1);
        ASSERT_EQ(c.columns [0].type, tdsl::detail::e_tds_data_type::BIGVARCHRTYPE);
        ASSERT_EQ(r [0].as<tdsl::char_view>().size(), 3);
        ASSERT_EQ(r [0].as<tdsl::char_view>().size_bytes(), 3);
    };

    // Create the table
    command_ctx.execute_query(tdsl::string_view{"CREATE TABLE #test_rpc(a varchar(255))"});
    // Fill some data
    command_ctx.execute_query(tdsl::string_view{"INSERT INTO #test_rpc VALUES('abc')"});

    command_ctx.execute_rpc(tdsl::string_view{"SELECT * FROM #test_rpc WHERE a=@p0"}, params,
                            tdsl::detail::e_rpc_mode::executesql, validator,
                            &validator_called_times);
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------

TEST_F(tds_command_ctx_it_fixture, test_rpc_nvarchar) {
    tdsl::detail::sql_parameter_nvarchar p1{};
    p1                                            = tdsl::wstring_view{u"abc"};

    // Text types does not support variable update and must be rebound
    // to parameter binding again.

    // The binding order determines the variable name, e.g. bound parameter 0 is p0
    // and bound parameter 1 is p1 and so on.
    tdsl::detail::sql_parameter_binding params [] = {p1};

    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 1);
        ASSERT_EQ(r.size(), 1);
        ASSERT_EQ(c.columns [0].type, tdsl::detail::e_tds_data_type::NVARCHARTYPE);
        ASSERT_EQ(r [0].as<tdsl::u16char_view>().size(), 3);
        ASSERT_EQ(r [0].as<tdsl::u16char_view>().size_bytes(), 6);
    };

    // Create the table
    command_ctx.execute_query(tdsl::string_view{"CREATE TABLE #test_rpc(a nvarchar(255))"});
    // Fill some data
    command_ctx.execute_query(tdsl::string_view{"INSERT INTO #test_rpc VALUES(N'abc')"});

    command_ctx.execute_rpc(tdsl::string_view{"SELECT * FROM #test_rpc WHERE a=@p0"}, params,
                            tdsl::detail::e_rpc_mode::executesql, validator,
                            &validator_called_times);
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------

TEST_F(tds_command_ctx_it_fixture, test_rpc_float) {
    tdsl::detail::sql_parameter_float4 p1{};
    // tdsl::detail::sql_parameter_float8 p2{};
    p1                                            = float{0.3f};
    // FIXME: Enable this back!
    // p2                                            = double{2.4};

    // Text types does not support variable update and must be rebound
    // to parameter binding again.

    // The binding order determines the variable name, e.g. bound parameter 0 is p0
    // and bound parameter 1 is p1 and so on.
    tdsl::detail::sql_parameter_binding params [] = {p1};
    tdsl::size_t validator_called_times           = {0};
    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 2);
        ASSERT_EQ(r.size(), 2);
        ASSERT_EQ(c.columns [0].type, tdsl::detail::e_tds_data_type::FLTNTYPE);
        ASSERT_EQ(c.columns [0].typeprops.u8l.length, 4);
        ASSERT_EQ(c.columns [1].type, tdsl::detail::e_tds_data_type::FLTNTYPE);
        ASSERT_EQ(c.columns [1].typeprops.u8l.length, 8);
        ASSERT_EQ(r [0].size_bytes(), 4);
        ASSERT_EQ(r [0].as<float>(), float{0.3f});
        ASSERT_EQ(r [1].size_bytes(), 8);
        ASSERT_EQ(r [1].as<double>(), double{0.3});
    };

    // Create the table
    auto r1 =
        command_ctx.execute_query(tdsl::string_view{"CREATE TABLE #test_rpc(a real, b float)"});

    ASSERT_TRUE(r1);
    ASSERT_FALSE(r1.status.count_valid());
    ASSERT_FALSE(r1.status.attn());
    ASSERT_FALSE(r1.status.error());
    ASSERT_FALSE(r1.status.in_xact());
    ASSERT_FALSE(r1.status.more());
    ASSERT_FALSE(r1.status.srverror());
    ASSERT_EQ(0, r1.affected_rows);

    // Fill some data
    auto r2 =
        command_ctx.execute_query(tdsl::string_view{"INSERT INTO #test_rpc VALUES(0.3, 0.3)"});
    ASSERT_TRUE(r2);
    ASSERT_TRUE(r2.status.count_valid());
    ASSERT_FALSE(r2.status.attn());
    ASSERT_FALSE(r2.status.error());
    ASSERT_FALSE(r2.status.in_xact());
    ASSERT_FALSE(r2.status.more());
    ASSERT_FALSE(r2.status.srverror());
    ASSERT_EQ(1, r2.affected_rows);

    command_ctx.execute_rpc(tdsl::string_view{"SELECT * FROM #test_rpc WHERE a=@p0"}, params,
                            tdsl::detail::e_rpc_mode::executesql, validator,
                            &validator_called_times);
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------

TEST_F(tds_command_ctx_it_fixture, test_rpc_guid) {
    tdsl::detail::sql_parameter_guid p1{};

    static constexpr std::uint8_t buf [] = {0x76, 0x6D, 0xE8, 0xE2, 0xE1, 0x8F, 0x28, 0x47,
                                            0xAC, 0x9D, 0x1C, 0xED, 0x7E, 0xFE, 0xA9, 0xD2};
    static constexpr tdsl::byte_view bv{buf};
    p1                                            = bv;

    tdsl::detail::sql_parameter_binding params [] = {p1};
    tdsl::size_t validator_called_times           = {0};
    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 1);
        ASSERT_EQ(r.size(), 1);
        ASSERT_EQ(c.columns [0].type, tdsl::detail::e_tds_data_type::GUIDTYPE);
        ASSERT_EQ(c.columns [0].typeprops.u8l.length, 16);
        ASSERT_EQ(r [0].size_bytes(), 16);
        ASSERT_THAT(r [0].as<tdsl::byte_view>(), testing::ElementsAreArray(bv));
    };

    // Create the table
    auto r1 =
        command_ctx.execute_query(tdsl::string_view{"CREATE TABLE #test_rpc(a uniqueidentifier)"});

    ASSERT_TRUE(r1);
    ASSERT_FALSE(r1.status.count_valid());
    ASSERT_FALSE(r1.status.attn());
    ASSERT_FALSE(r1.status.error());
    ASSERT_FALSE(r1.status.in_xact());
    ASSERT_FALSE(r1.status.more());
    ASSERT_FALSE(r1.status.srverror());
    ASSERT_EQ(0, r1.affected_rows);

    // Fill some data
    auto r2 = command_ctx.execute_query(
        tdsl::string_view{"INSERT INTO #test_rpc VALUES('E2E86D76-8FE1-4728-AC9D-1CED7EFEA9D2')"});
    ASSERT_TRUE(r2);
    ASSERT_TRUE(r2.status.count_valid());
    ASSERT_FALSE(r2.status.attn());
    ASSERT_FALSE(r2.status.error());
    ASSERT_FALSE(r2.status.in_xact());
    ASSERT_FALSE(r2.status.more());
    ASSERT_FALSE(r2.status.srverror());
    ASSERT_EQ(1, r2.affected_rows);

    command_ctx.execute_rpc(tdsl::string_view{"SELECT * FROM #test_rpc WHERE a=@p0"}, params,
                            tdsl::detail::e_rpc_mode::executesql, validator,
                            &validator_called_times);
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------

TEST_F(tds_command_ctx_it_fixture, test_rpc_char) {
    tdsl::detail::sql_parameter_char p1{};
    p1                                            = tdsl::string_view{"abc"};

    tdsl::detail::sql_parameter_binding params [] = {p1};

    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 1);
        ASSERT_EQ(r.size(), 1);
        ASSERT_EQ(c.columns [0].type, tdsl::detail::e_tds_data_type::BIGCHARTYPE);
        ASSERT_EQ(r [0].as<tdsl::char_view>().size(), 3);
        ASSERT_EQ(r [0].as<tdsl::char_view>().size_bytes(), 3);
    };

    // Create the table
    command_ctx.execute_query(tdsl::string_view{"CREATE TABLE #test_rpc(a char(3))"});
    // Fill some data
    command_ctx.execute_query(tdsl::string_view{"INSERT INTO #test_rpc VALUES('abc')"});

    command_ctx.execute_rpc(tdsl::string_view{"SELECT * FROM #test_rpc WHERE a=@p0"}, params,
                            tdsl::detail::e_rpc_mode::executesql, validator,
                            &validator_called_times);
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------

TEST_F(tds_command_ctx_it_fixture, test_rpc_nchar) {
    tdsl::detail::sql_parameter_nchar p1{};
    p1                                            = tdsl::wstring_view{u"abc"};

    tdsl::detail::sql_parameter_binding params [] = {p1};

    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 1);
        ASSERT_EQ(r.size(), 1);
        ASSERT_EQ(c.columns [0].type, tdsl::detail::e_tds_data_type::NCHARTYPE);
        ASSERT_EQ(r [0].as<tdsl::u16char_view>().size(), 3);
        ASSERT_EQ(r [0].as<tdsl::u16char_view>().size_bytes(), 6);
    };

    // Create the table
    command_ctx.execute_query(tdsl::string_view{"CREATE TABLE #test_rpc(a nchar(3))"});
    // Fill some data
    command_ctx.execute_query(tdsl::string_view{"INSERT INTO #test_rpc VALUES(N'abc')"});

    command_ctx.execute_rpc(tdsl::string_view{"SELECT * FROM #test_rpc WHERE a=@p0"}, params,
                            tdsl::detail::e_rpc_mode::executesql, validator,
                            &validator_called_times);
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------

TEST_F(tds_command_ctx_it_fixture, test_rpc_binary) {
    tdsl::detail::sql_parameter_binary p1{};
    tdsl::uint8_t binary []                       = {0x01, 0x02, 0x03, 0x04, 0x05};
    p1                                            = tdsl::byte_view{binary};

    tdsl::detail::sql_parameter_binding params [] = {p1};

    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 1);
        ASSERT_EQ(r.size(), 1);
        ASSERT_EQ(c.columns [0].type, tdsl::detail::e_tds_data_type::BIGBINARYTYPE);
        ASSERT_EQ(r [0].as<tdsl::byte_view>().size(), 5);
        ASSERT_EQ(r [0].as<tdsl::byte_view>().size_bytes(), 5);
    };

    // Create the table
    command_ctx.execute_query(tdsl::string_view{"CREATE TABLE #test_rpc(a binary(5))"});
    // Fill some data
    command_ctx.execute_query(tdsl::string_view{"INSERT INTO #test_rpc VALUES(0x0102030405)"});

    command_ctx.execute_rpc(tdsl::string_view{"SELECT * FROM #test_rpc WHERE a=@p0"}, params,
                            tdsl::detail::e_rpc_mode::executesql, validator,
                            &validator_called_times);
    ASSERT_EQ(1, validator_called_times);
}

// --------------------------------------------------------------------------------

TEST_F(tds_command_ctx_it_fixture, test_rpc_varbinary) {
    tdsl::detail::sql_parameter_varbinary p1{};
    tdsl::uint8_t binary []                       = {0x01, 0x02, 0x03, 0x04, 0x05};
    p1                                            = tdsl::byte_view{binary};

    tdsl::detail::sql_parameter_binding params [] = {p1};

    auto validator = +[](void * b, uut_t::column_metadata_cref c, uut_t::row_cref r) {
        validator_called(b);
        ASSERT_EQ(c.columns.size(), 1);
        ASSERT_EQ(r.size(), 1);
        ASSERT_EQ(c.columns [0].type, tdsl::detail::e_tds_data_type::BIGVARBINTYPE);
        ASSERT_EQ(r [0].as<tdsl::byte_view>().size(), 5);
        ASSERT_EQ(r [0].as<tdsl::byte_view>().size_bytes(), 5);
    };

    // Create the table
    command_ctx.execute_query(tdsl::string_view{"CREATE TABLE #test_rpc(a varbinary(5))"});
    // Fill some data
    command_ctx.execute_query(tdsl::string_view{"INSERT INTO #test_rpc VALUES(0x0102030405)"});

    command_ctx.execute_rpc(tdsl::string_view{"SELECT * FROM #test_rpc WHERE a=@p0"}, params,
                            tdsl::detail::e_rpc_mode::executesql, validator,
                            &validator_called_times);
    ASSERT_EQ(1, validator_called_times);
}
