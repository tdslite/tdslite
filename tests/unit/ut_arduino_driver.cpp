/**
 * _________________________________________________
 * Unit tests for tdslite-net-arduino
 *
 * @file   ut_arduino_driver.cpp
 * @author mkg <me@mustafagilor.com>
 * @date   08.03.2023
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#include <functional>

std::function<void(int)> delay = [](int ms) {
    (void) ms;
};

unsigned long millis() {
    return 0;
}

#include <tdslite-net/arduino/tdsl_netimpl_arduino.hpp>
#include <tdslite/util/tdsl_string_view.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

struct my_client {

    struct ip_addr {
        uint8_t _address [4]; // IPv4 address

        // Access the raw byte array containing the address.  Because this returns a pointer
        // to the internal structure rather than a copy of the address this function should only
        // be used when you know that the usage of the returned uint8_t* will be transient and not
        // stored.
        uint8_t * raw_address() {
            return _address;
        };

    public:
        // Constructors
        ip_addr();

        ip_addr(tdsl::uint8_t first_octet, tdsl::uint8_t second_octet, tdsl::uint8_t third_octet,
                tdsl::uint8_t fourth_octet) :
            _address{first_octet, second_octet, third_octet, fourth_octet} {}

        tdsl::uint8_t operator[](int index) const {
            return _address [index];
        };

        tdsl::uint8_t & operator[](int index) {
            return _address [index];
        };
    };

    tdsl::uint8_t cb [1024] = {
        0x10, 0x01, 0x00, 0xc4, 0x00, 0x00, 0x01, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x01, 0x00,
        0x00, 0x71, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x56, 0x00, 0x0a, 0x00, 0x6a, 0x00, 0x07, 0x00, 0x78, 0x00, 0x07, 0x00,
        0x86, 0x00, 0x04, 0x00, 0x8e, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa6, 0x00,
        0x04, 0x00, 0xae, 0x00, 0x00, 0x00, 0xae, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x41, 0x00, 0x4c, 0x00,
        0x2d, 0x00, 0x44, 0x00, 0x45, 0x00, 0x4c, 0x00, 0x4c, 0x00, 0x2d, 0x00, 0x30, 0x00,
        0x32, 0x00, 0x4a, 0x00, 0x61, 0x00, 0x78, 0x00, 0x56, 0x00, 0x69, 0x00, 0x65, 0x00,
        0x77, 0x00, 0x01, 0xa5, 0xb3, 0xa5, 0x22, 0xa5, 0xc0, 0xa5, 0x33, 0xa5, 0xf3, 0xa5,
        0xd2, 0xa5, 0x6a, 0x00, 0x54, 0x00, 0x44, 0x00, 0x53, 0x00, 0x31, 0x00, 0x39, 0x00,
        0x32, 0x00, 0x2e, 0x00, 0x31, 0x00, 0x36, 0x00, 0x38, 0x00, 0x2e, 0x00, 0x32, 0x00,
        0x2e, 0x00, 0x33, 0x00, 0x38, 0x00, 0x6a, 0x00, 0x54, 0x00, 0x44, 0x00, 0x53, 0x00,
        0x4a, 0x00, 0x61, 0x00, 0x78, 0x00, 0x56, 0x00, 0x69, 0x00, 0x65, 0x00, 0x77, 0x00};

    tdsl::size_t avail = sizeof(cb);

    tdsl::size_t write(const unsigned char * buf, tdsl::size_t len) {
        (void) buf;
        (void) len;
        return len;
    }

    int connect(const char *, unsigned short a) {
        return a - 100;
    }

    virtual int read(unsigned char * buf, unsigned long amount) {
        if (amount > sizeof(cb)) {
            TDSL_CANNOT_HAPPEN;
        }

        memcpy(buf, cb + (sizeof(cb) - avail), amount);
        avail -= amount;
        return static_cast<int>(amount);
    }

    bool connected() {
        return true;
    }

    int available() {
        return avail;
    }

    void flush() {}

    void stop() {}

    tdsl::uint16_t localPort() {
        return 2000;
    }

    tdsl::uint16_t remotePort() {
        return 2001;
    }

    ip_addr localIP() {
        return ip_addr(192, 168, 2, 1);
    }

    ip_addr remoteIP() {
        return ip_addr(192, 168, 3, 1);
    }

    void reset_avail() {
        avail = sizeof(cb);
    }
};

struct my_client_chunked_read : public my_client {
    virtual int read(unsigned char * buf, unsigned long amount) override {
        if (amount > sizeof(cb)) {
            TDSL_CANNOT_HAPPEN;
        }

        if (amount > 1) {
            amount = 1;
        }

        memcpy(buf, cb + (sizeof(cb) - avail), amount);
        avail -= amount;
        return static_cast<int>(amount);
    }
};

tdsl::uint8_t buf [512] = {0};

template <typename ClientType>
using uut_t = tdsl::net::tdsl_netimpl_arduino<ClientType>;

TEST(test, construct) {
    uut_t<my_client> the_client{buf};
}

TEST(test, connect) {
    uut_t<my_client> the_client{buf};
    for (int i = 0; i < 200; i++) {
        auto r = the_client.connect(/*host=*/tdsl::string_view{/*str=*/"a"}, /*port=*/i);
        if (i == 101) {
            ASSERT_TRUE(static_cast<bool>(r));
        }
        else {
            ASSERT_FALSE(static_cast<bool>(r));
        }
    }
}

TEST(test, connect_retry) {
    auto delay_original = delay;
    testing::MockFunction<void(int)> mock_delay;
    EXPECT_CALL(mock_delay, Call(1234)).Times(15);
    delay = mock_delay.AsStdFunction();
    uut_t<my_client> the_client{buf};
    the_client.set_connection_timeout_params(/*attempts=*/15, /*delay_ms=*/1234);
    auto r = the_client.connect(/*host=*/tdsl::string_view{/*str=*/"a"}, /*port=*/105);
    ASSERT_FALSE(r);
    delay = delay_original;
}

TEST(test, write) {
    uut_t<my_client> the_client{buf};
    the_client.do_write(tdsl::span<tdsl::uint8_t>{/*begin=*/nullptr, /*end=*/nullptr});
}

TEST(test, receive_tds_pdu) {
    uut_t<my_client> the_client{buf};
    ASSERT_EQ(1, the_client.do_receive_tds_pdu());
}

TEST(test, receive_tds_pdu_chunked) {
    uut_t<my_client_chunked_read> the_client{buf};
    ASSERT_EQ(1, the_client.do_receive_tds_pdu());
}

TEST(test, send_tds_pdu) {
    uut_t<my_client> the_client{buf};
    the_client.do_send_tds_pdu(tdsl::detail::e_tds_message_type::login);
}