/**
 * ____________________________________________________
 * Unit tests for tdslite command context
 *
 * @file   ut_tdsl_command_context.cpp
 * @author mkg <me@mustafagilor.com>
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <string>
#include <algorithm>
#include <tdslite/detail/tdsl_command_context.hpp>
#include <tdslite/util/tdsl_hex_dump.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <cstring>
#include <array>

namespace {

    struct mock_network_impl {

        template <typename T>
        inline void do_write(tdsl::span<T> data) noexcept {
            send_buffer.insert(send_buffer.end(), data.begin(), data.end());
        }

        template <typename T>
        inline void do_write(tdsl::size_t offset, tdsl::span<T> data) noexcept {

            auto beg = std::next(send_buffer.begin(), offset);
            auto end = std::next(beg, data.size_bytes());
            if (beg >= send_buffer.end() || end > send_buffer.end()) {
                return;
            }

            std::copy(data.begin(), data.end(), beg);
        }

        /**
         * Get current write offset
         */
        inline tdsl::size_t do_get_write_offset() noexcept {
            return send_buffer.size();
        }

        inline void do_send(void) noexcept {}

        inline tdsl::detail::e_send_status
        do_send_tds_pdu(tdsl::detail::e_tds_message_type) noexcept {
            return tdsl::detail::e_send_status::success;
        }

        inline void do_receive_tds_pdu() {}

        inline void set_tds_packet_size(tdsl::uint16_t) {}

        void register_packet_data_callback(
            tdsl::uint32_t (*)(void *, tdsl::detail::e_tds_message_type,
                               tdsl::binary_reader<tdsl::endian::little> &),
            void *) {}

        std::vector<uint8_t> send_buffer;
    };
} // namespace

// --------------------------------------------------------------------------------

/**
 * The type of the unit-under-test
 */
using uut_t     = tdsl::detail::command_context<mock_network_impl>;

using tds_ctx_t = uut_t::tds_context_type;

struct tdsl_command_ctx_ut_fixture : public ::testing::Test {

    virtual void TearDown() override {
        tdsl::util::hexdump(tds_ctx.send_buffer.data(), tds_ctx.send_buffer.size());
    }

    tds_ctx_t tds_ctx;

    uut_t command_ctx{tds_ctx};
};

// --------------------------------------------------------------------------------

TEST_F(tdsl_command_ctx_ut_fixture, test_01) {
    command_ctx.execute_query(tdsl::string_view{"SELECT * FROM FOO;"});

    constexpr std::array<tdsl::uint8_t, 36> expected_packet_bytes{
        0x53, 0x00, 0x45, 0x00, 0x4c, 0x00, 0x45, 0x00, 0x43, 0x00, 0x54, 0x00,
        0x20, 0x00, 0x2a, 0x00, 0x20, 0x00, 0x46, 0x00, 0x52, 0x00, 0x4f, 0x00,
        0x4d, 0x00, 0x20, 0x00, 0x46, 0x00, 0x4f, 0x00, 0x4f, 0x00, 0x3b, 0x00};

    ASSERT_EQ(sizeof(expected_packet_bytes), tds_ctx.send_buffer.size());
    ASSERT_EQ(sizeof(expected_packet_bytes), tds_ctx.send_buffer.size());
    ASSERT_THAT(tds_ctx.send_buffer, testing::ElementsAreArray(expected_packet_bytes));
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_command_ctx_ut_fixture, test_rpc) {

    tdsl::detail::sql_parameter_tinyint p1;
    tdsl::detail::sql_parameter_smallint p2;
    tdsl::detail::sql_parameter_int p3;
    tdsl::detail::sql_parameter_bigint p4         = 5;
    p4                                            = tdsl::int64_t{5};
    tdsl::detail::sql_parameter_binding params [] = {p1, p2, p3, p4};
    tdsl::uint8_t b                               = p1;
    (void) b;

    // params[0].value =
    command_ctx.execute_rpc(tdsl::string_view{"SELECT * FROM TEST WHERE a=@P1"}, params);

    // This data begins after TDS header
    std::array<tdsl::uint8_t, 6> header{// Procedure name length
                                        0xFF, 0xFF,
                                        // Special Procedure ID
                                        0x0a, 0x00, // sp_executesql(10)
                                                    // Option flags
                                        0x00, 0x00};
    std::array<tdsl::uint8_t, 10> query_typeinfo{// Name length, status flags
                                                 0x00, 0x00,
                                                 // Type (NVARCHAR)
                                                 0xE7,
                                                 // Max length (8000)
                                                 0x40, 0x1f,
                                                 // Collation info
                                                 0x00, 0x00, 0x00, 0x00, 0x00};

    tdsl::wstring_view query{u"SELECT * FROM TEST WHERE a=@P1"};
    const tdsl::uint16_t query_bytelen = tdsl::native_to_le(query.size_bytes());

    std::array<tdsl::uint8_t, 10> vardecl_typeinfo{// Name length, status flags
                                                   0x00, 0x00,
                                                   // Type (NVARCHAR)
                                                   0xE7,
                                                   // Max length (8000)
                                                   0x40, 0x1f,
                                                   // Collation info
                                                   0x00, 0x00, 0x00, 0x00, 0x00};
    tdsl::wstring_view vardecl{u"@p0 TINYINT,@p1 SMALLINT,@p2 INT,@p3 BIGINT"};
    const tdsl::uint16_t vardecl_bytelen = tdsl::native_to_le(vardecl.size_bytes());

    std::array<tdsl::uint8_t, 6> param_p0{
        0x00, 0x00, 0x26, 0x01, 0x01, 0x00,
    };
    std::array<tdsl::uint8_t, 7> param_p1{
        0x00, 0x00, 0x26, 0x02, 0x02, 0x00, 0x00,
    };
    std::array<tdsl::uint8_t, 9> param_p2{
        0x00, 0x00, 0x26, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00,
    };
    std::array<tdsl::uint8_t, 13> param_p3{0x00, 0x00, 0x26, 0x08, 0x08, 0x05, 0x00,
                                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    std::vector<tdsl::uint8_t> expected_packet_bytes{};

    expected_packet_bytes.insert(expected_packet_bytes.end(), header.begin(), header.end());
    expected_packet_bytes.insert(expected_packet_bytes.end(), query_typeinfo.begin(),
                                 query_typeinfo.end());
    expected_packet_bytes.insert(expected_packet_bytes.end(),
                                 reinterpret_cast<const tdsl::uint8_t *>(&query_bytelen),
                                 reinterpret_cast<const tdsl::uint8_t *>(&query_bytelen) + 2);
    expected_packet_bytes.insert(expected_packet_bytes.end(),
                                 query.rebind_cast<const char>().begin(),
                                 query.rebind_cast<const char>().end());
    expected_packet_bytes.insert(expected_packet_bytes.end(), vardecl_typeinfo.begin(),
                                 vardecl_typeinfo.end());
    expected_packet_bytes.insert(expected_packet_bytes.end(),
                                 reinterpret_cast<const tdsl::uint8_t *>(&vardecl_bytelen),
                                 reinterpret_cast<const tdsl::uint8_t *>(&vardecl_bytelen) + 2);
    expected_packet_bytes.insert(expected_packet_bytes.end(),
                                 vardecl.rebind_cast<const char>().begin(),
                                 vardecl.rebind_cast<const char>().end());

    expected_packet_bytes.insert(expected_packet_bytes.end(), param_p0.begin(), param_p0.end());
    expected_packet_bytes.insert(expected_packet_bytes.end(), param_p1.begin(), param_p1.end());
    expected_packet_bytes.insert(expected_packet_bytes.end(), param_p2.begin(), param_p2.end());
    expected_packet_bytes.insert(expected_packet_bytes.end(), param_p3.begin(), param_p3.end());

    EXPECT_THAT(tds_ctx.send_buffer, testing::ElementsAreArray(expected_packet_bytes));
    // Expected
    tdsl::util::hexdump(expected_packet_bytes.data(), expected_packet_bytes.size());
}

// --------------------------------------------------------------------------------

/**
 * Regression test: the command context must unregister the callbacks it
 * registered into the (longer-living) tds_context when it is destroyed.
 * Otherwise, a later DONE token (e.g. one received during a re-login)
 * is delivered to a dangling pointer and corrupts the stack.
 */
TEST(tdsl_command_ctx_ut, callbacks_unregistered_on_destruction) {
    tds_ctx_t tds_ctx;
    {
        uut_t command_ctx{tds_ctx};
        EXPECT_TRUE(tds_ctx.callbacks.done);
        EXPECT_TRUE(tds_ctx.callbacks.sub_token_handler);
    }
    EXPECT_FALSE(tds_ctx.callbacks.done);
    EXPECT_FALSE(tds_ctx.callbacks.sub_token_handler);

    // Must be a no-op now (was a stack-use-after-scope write before the fix)
    tds_ctx.callbacks.done(tdsl::tds_done_token{});
}

// --------------------------------------------------------------------------------
// RPC parameter declarations and validation
// --------------------------------------------------------------------------------

namespace {

    /**
     * UTF-16LE bytes of an ASCII string
     */
    inline std::vector<tdsl::uint8_t> u16le(const std::string & s) {
        std::vector<tdsl::uint8_t> r;
        for (const char c : s) {
            r.push_back(static_cast<tdsl::uint8_t>(c));
            r.push_back(0x00);
        }
        return r;
    }

    /**
     * Whether the sent request contains the parameter declaration
     * string @p decl
     */
    inline bool request_contains(const std::vector<tdsl::uint8_t> & buf, const std::string & decl) {
        const auto needle = u16le(decl);
        return std::search(buf.begin(), buf.end(), needle.begin(), needle.end()) != buf.end();
    }

    struct rpc_param_fixture : public ::testing::Test {
        uut_t::execute_rpc_result rpc(tdsl::detail::sql_parameter_binding pb) {
            tdsl::detail::sql_parameter_binding params [] = {pb};
            return command_ctx.execute_rpc(tdsl::string_view{"SELECT @p0"}, params);
        }

        tds_ctx_t tds_ctx;
        uut_t command_ctx{tds_ctx};
    };

    const std::u16string k_hello = u"hello";
} // namespace

// The NVARCHAR length used to be the byte count (NVARCHAR(10) for 5 chars)
TEST_F(rpc_param_fixture, nvarchar_length_is_in_characters) {
    tdsl::detail::sql_parameter_nvarchar p{};
    p = tdsl::wstring_view{k_hello.data(), static_cast<tdsl::uint32_t>(k_hello.size())};
    (void) rpc(p);
    EXPECT_TRUE(request_contains(tds_ctx.send_buffer, "@p0 NVARCHAR(5)"));
}

// Empty strings used to be declared as NVARCHAR(0)/VARCHAR(0), which are invalid
TEST_F(rpc_param_fixture, empty_string_is_declared_with_length_one) {
    tdsl::detail::sql_parameter_nvarchar p{};
    p = tdsl::wstring_view{u""};
    (void) rpc(p);
    EXPECT_TRUE(request_contains(tds_ctx.send_buffer, "@p0 NVARCHAR(1)"));

    tds_ctx.send_buffer.clear();
    tdsl::detail::sql_parameter_varchar v{};
    v = tdsl::string_view{""};
    (void) rpc(v);
    EXPECT_TRUE(request_contains(tds_ctx.send_buffer, "@p0 VARCHAR(1)"));
}

TEST_F(rpc_param_fixture, longest_nvarchar_is_accepted) {
    const std::u16string s(4000, u'x');
    tdsl::detail::sql_parameter_nvarchar p{};
    p = tdsl::wstring_view{s.data(), static_cast<tdsl::uint32_t>(s.size())};
    (void) rpc(p);
    EXPECT_TRUE(request_contains(tds_ctx.send_buffer, "@p0 NVARCHAR(4000)"));
}

// Values longer than 8000 bytes used to be sent with a truncated length
TEST_F(rpc_param_fixture, too_long_values_are_rejected) {
    const std::u16string s(4001, u'x');
    tdsl::detail::sql_parameter_nvarchar p{};
    p            = tdsl::wstring_view{s.data(), static_cast<tdsl::uint32_t>(s.size())};
    const auto r = rpc(p);
    ASSERT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(tdsl::detail::e_rpc_error_code::invalid_parameter, r.error());
    EXPECT_TRUE(tds_ctx.send_buffer.empty()) << "nothing must be written";

    const std::vector<tdsl::uint8_t> b(8001, 0x01);
    tdsl::detail::sql_parameter_varbinary vb{};
    vb            = tdsl::byte_view{b.data(), static_cast<tdsl::uint32_t>(b.size())};
    const auto r2 = rpc(vb);
    ASSERT_FALSE(static_cast<bool>(r2));
    EXPECT_EQ(tdsl::detail::e_rpc_error_code::invalid_parameter, r2.error());
}

// These used to trap (TDSL_CANNOT_HAPPEN / TDSL_NOT_YET_IMPLEMENTED)
TEST_F(rpc_param_fixture, unsupported_parameters_are_rejected) {
    const tdsl::uint8_t three [3]                    = {1, 2, 3};
    const tdsl::uint8_t five [5]                     = {1, 0, 0, 0, 0};
    const tdsl::uint8_t four [4]                     = {0x61, 0x00, 0x62, 0x00};

    const tdsl::detail::sql_parameter_binding bad [] = {
        // INTN with an invalid size
        {tdsl::detail::e_tds_data_type::INTNTYPE,          tdsl::byte_view{three}, 3},
        // DECIMAL
        {tdsl::detail::e_tds_data_type::DECIMALNTYPE,      tdsl::byte_view{five},  5},
        // TEXT
        {tdsl::detail::e_tds_data_type::NTEXTTYPE,         tdsl::byte_view{four},  4},
        // Unknown type
        {static_cast<tdsl::detail::e_tds_data_type>(0xF1), tdsl::byte_view{four},  4},
        // GUID with a wrong size
        {tdsl::detail::e_tds_data_type::GUIDTYPE,          tdsl::byte_view{four},  4},
        // NVARCHAR with an odd byte count
        {tdsl::detail::e_tds_data_type::NVARCHARTYPE,      tdsl::byte_view{three}, 3},
    };
    for (const auto & pb : bad) {
        SCOPED_TRACE(static_cast<int>(pb.type));
        tds_ctx.send_buffer.clear();
        const auto r = rpc(pb);
        ASSERT_FALSE(static_cast<bool>(r));
        EXPECT_EQ(tdsl::detail::e_rpc_error_code::invalid_parameter, r.error());
        EXPECT_TRUE(tds_ctx.send_buffer.empty());
    }
}

// MONEY and BIT(N) used to trap in the declaration writer
TEST_F(rpc_param_fixture, money_and_bitn_parameters_are_declared) {
    const tdsl::uint8_t eight [8] = {0};
    const tdsl::uint8_t one [1]   = {1};
    (void) rpc({tdsl::detail::e_tds_data_type::MONEYNTYPE, tdsl::byte_view{eight}, 8});
    EXPECT_TRUE(request_contains(tds_ctx.send_buffer, "@p0 MONEY"));
    tds_ctx.send_buffer.clear();
    (void) rpc({tdsl::detail::e_tds_data_type::BITNTYPE, tdsl::byte_view{one}, 1});
    EXPECT_TRUE(request_contains(tds_ctx.send_buffer, "@p0 BIT"));
}

// prepexec used to trap (TDSL_NOT_YET_IMPLEMENTED)
TEST_F(rpc_param_fixture, prepexec_mode_is_rejected) {
    const auto r = command_ctx.execute_rpc(tdsl::string_view{"SELECT 1"}, {},
                                           tdsl::detail::e_rpc_mode::prepexec);
    ASSERT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(tdsl::detail::e_rpc_error_code::invalid_mode, r.error());
    EXPECT_TRUE(tds_ctx.send_buffer.empty());
}

// The command is sent as NVARCHAR(4000)
TEST_F(rpc_param_fixture, command_longer_than_4000_chars_is_rejected) {
    const std::string cmd(4001, ' ');
    const auto r = command_ctx.execute_rpc(
        tdsl::string_view{cmd.data(), static_cast<tdsl::uint32_t>(cmd.size())});
    ASSERT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(tdsl::detail::e_rpc_error_code::request_too_large, r.error());
    EXPECT_TRUE(tds_ctx.send_buffer.empty());
}
