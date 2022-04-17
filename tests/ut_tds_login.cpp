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

#include <vector>

void hexdump(void * ptr, int buflen) {
    unsigned char * buf = static_cast<unsigned char *>(ptr);
    for (int i = 0; i < buflen; i += 16) {
        printf("%06x: ", i);
        for (int j = 0; j < 16; j++)
            if (i + j < buflen)
                printf("%02x ", buf [i + j]);
            else
                printf("   ");
        printf(" ");
        for (int j = 0; j < 16; j++)
            if (i + j < buflen)
                printf("%c", isprint(buf [i + j]) ? buf [i + j] : '.');
        printf("\n");
    }
}

struct mock_network_impl {
    inline void do_write(tdslite::span<const tdslite::uint8_t> data) noexcept {
        buffer.insert(buffer.end(), data.begin(), data.end());
    }

    inline void do_write(tdslite::span<const char> data) noexcept {
        buffer.insert(buffer.end(), data.begin(), data.end());
    }

    inline void do_send(void) noexcept {}

    std::vector<uint8_t> buffer;
};

TEST(test, test) {
    tdslite::tds::login_context<mock_network_impl> login;
    tdslite::tds::login_context<mock_network_impl>::login_parameters params;
    params.server_name = "localhost";
    params.db_name     = "test";
    params.user_name   = "sa";
    params.password    = "test";
    params.client_name = "unit test";
    login.do_login(params);

    hexdump(login.buffer.data(), login.buffer.size());
    return;
}