/**
 * _________________________________________________
 * Unit tests for tdslite command context
 *
 * @file   ut_tdsl_command_context.cpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

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

        inline void do_send_tds_pdu(tdsl::detail::e_tds_message_type) noexcept {}

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