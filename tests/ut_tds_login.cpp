/**
 * _________________________________________________
 * Unit tests for tds login helper
 *
 * @file   ut_tds_login.cpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   14.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#include <tdslite/tds/tds_login_context.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <cstring>
#include <array>

namespace {

    void hexdump(const void * ptr, unsigned long long buflen) {
        const unsigned char * buf = static_cast<const unsigned char *>(ptr);
        for (unsigned long i = 0; i < buflen; i += 16) {
            printf("%06lx: ", i);
            for (unsigned long j = 0; j < 16; j++)
                if (i + j < buflen)
                    printf("%02x ", buf [i + j]);
                else
                    printf("   ");
            printf(" ");
            for (unsigned long j = 0; j < 16; j++)
                if (i + j < buflen)
                    printf("%c", isprint(buf [i + j]) ? buf [i + j] : '.');
            printf("\n");
        }
    }

    struct mock_network_impl {

        template <typename T>
        inline void do_write(tdslite::span<T> data) noexcept {
            buffer.insert(buffer.end(), data.begin(), data.end());
        }

        template <typename T>
        inline void do_write(tdslite::uint32_t offset, tdslite::span<T> data) noexcept {

            auto beg = std::next(buffer.begin(), offset);
            auto end = std::next(beg, data.size_bytes());
            if (beg >= buffer.end() || end > buffer.end()) {
                return;
            }

            std::copy(data.begin(), data.end(), beg);
        }

        inline void do_send(void) noexcept {}

        std::vector<uint8_t> buffer;
    };
} // namespace

// --------------------------------------------------------------------------------

/**
 * The type of the unit-under-test
 */
using uut_t = tdslite::tds::login_context<mock_network_impl>;

struct tds_login_fixture : public ::testing::Test {

    virtual void TearDown() override {
        hexdump(login.buffer.data(), login.buffer.size());
    }

    uut_t login;
};

// --------------------------------------------------------------------------------

TEST_F(tds_login_fixture, encode_password) {

    char16_t buf [] = u"JaxView";
    EXPECT_NO_THROW(uut_t::encode_password(reinterpret_cast<tdslite::uint8_t(&) [sizeof(buf) - 2]>(buf)));

    constexpr tdslite::uint8_t expected_buf [] = {0x01, 0xa5, 0xb3, 0xa5, 0x22, 0xa5, 0xc0, 0xa5,
                                                  0x33, 0xa5, 0xf3, 0xa5, 0xd2, 0xa5

    };

    EXPECT_EQ(std::memcmp(buf, expected_buf, sizeof(expected_buf)), 0);
}

// --------------------------------------------------------------------------------

TEST_F(tds_login_fixture, test_01) {

    uut_t::login_parameters params;
    params.server_name = "localhost";
    params.db_name     = "test";
    params.user_name   = "sa";
    params.password    = "test";
    params.client_name = "unit test";
    login.do_login(params);
}

// --------------------------------------------------------------------------------

TEST_F(tds_login_fixture, test_jaxview) {
    uut_t::login_parameters params;
    params.server_name  = "192.168.2.38";
    params.db_name      = "JaxView";
    params.user_name    = "JaxView";
    params.password     = "JaxView";
    params.client_name  = "AL-DELL-02";
    params.app_name     = "jTDS";
    params.library_name = "jTDS";

    constexpr std::array<tdslite::uint8_t, 196> expected_packet_bytes{
        0x10, 0x01, 0x00, 0xc4, 0x00, 0x00, 0x01, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x71, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00,
        0x00, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x56, 0x00, 0x0a, 0x00, 0x6a, 0x00, 0x07, 0x00, 0x78, 0x00, 0x07, 0x00, 0x86, 0x00, 0x04, 0x00, 0x8e, 0x00, 0x0c, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xa6, 0x00, 0x04, 0x00, 0xae, 0x00, 0x00, 0x00, 0xae, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbc, 0x00,
        0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x41, 0x00, 0x4c, 0x00, 0x2d, 0x00, 0x44, 0x00, 0x45, 0x00, 0x4c, 0x00, 0x4c, 0x00, 0x2d, 0x00,
        0x30, 0x00, 0x32, 0x00, 0x4a, 0x00, 0x61, 0x00, 0x78, 0x00, 0x56, 0x00, 0x69, 0x00, 0x65, 0x00, 0x77, 0x00, 0x01, 0xa5, 0xb3, 0xa5,
        0x22, 0xa5, 0xc0, 0xa5, 0x33, 0xa5, 0xf3, 0xa5, 0xd2, 0xa5, 0x6a, 0x00, 0x54, 0x00, 0x44, 0x00, 0x53, 0x00, 0x31, 0x00, 0x39, 0x00,
        0x32, 0x00, 0x2e, 0x00, 0x31, 0x00, 0x36, 0x00, 0x38, 0x00, 0x2e, 0x00, 0x32, 0x00, 0x2e, 0x00, 0x33, 0x00, 0x38, 0x00, 0x6a, 0x00,
        0x54, 0x00, 0x44, 0x00, 0x53, 0x00, 0x4a, 0x00, 0x61, 0x00, 0x78, 0x00, 0x56, 0x00, 0x69, 0x00, 0x65, 0x00, 0x77, 0x00};

    login.do_login(params);
    printf("sizeof char16_t %ld\n”", sizeof(char16_t));
    hexdump(&expected_packet_bytes [0], sizeof(expected_packet_bytes));
    printf("\n %ld vs. %ld \n", sizeof(expected_packet_bytes), login.buffer.size());

    ASSERT_EQ(sizeof(expected_packet_bytes), login.buffer.size());
    ASSERT_EQ(sizeof(expected_packet_bytes), login.buffer.size());
    ASSERT_THAT(login.buffer, testing::ElementsAreArray(expected_packet_bytes));
}