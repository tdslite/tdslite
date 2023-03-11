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

void delay(int ms) {
    (void) ms;
}

unsigned long millis() {
    return 0;
}

#include <tdslite/net/arduino/ethernet/tdsl_netimpl_arduino.hpp>
#include <tdslite/util/tdsl_string_view.hpp>
#include <gtest/gtest.h>

// class EthernetClient : public Client {
// public:
// 	EthernetClient() : _sockindex(MAX_SOCK_NUM), _timeout(1000) { }
// 	EthernetClient(uint8_t s) : _sockindex(s), _timeout(1000) { }
// 	virtual ~EthernetClient() {};

// 	uint8_t status();
// 	virtual int connect(IPAddress ip, uint16_t port);
// 	virtual int connect(const char *host, uint16_t port);
// 	virtual int availableForWrite(void);
// 	virtual size_t write(uint8_t);
// 	virtual size_t write(const uint8_t *buf, size_t size);
// 	virtual int available();
// 	virtual int read();
// 	virtual int read(uint8_t *buf, size_t size);
// 	virtual int peek();
// 	virtual void flush();
// 	virtual void stop();
// 	virtual uint8_t connected();
// 	virtual operator bool() { return _sockindex < MAX_SOCK_NUM; }
// 	virtual bool operator==(const bool value) { return bool() == value; }
// 	virtual bool operator!=(const bool value) { return bool() != value; }
// 	virtual bool operator==(const EthernetClient&);
// 	virtual bool operator!=(const EthernetClient& rhs) { return !this->operator==(rhs); }
// 	uint8_t getSocketNumber() const { return _sockindex; }
// 	virtual uint16_t localPort();
// 	virtual IPAddress remoteIP();
// 	virtual uint16_t remotePort();
// 	virtual void setConnectionTimeout(uint16_t timeout) { _timeout = timeout; }

// 	friend class EthernetServer;

// 	using Print::write;

// private:
// 	uint8_t _sockindex; // MAX_SOCK_NUM means client not in use
// 	uint16_t _timeout;
// };

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

    int connect(const char * host, unsigned short port) {
        (void) host;
        (void) port;
        return 0;
    }

    int read(unsigned char * buf, unsigned long amount) {
        if (amount > sizeof(cb)) {
            TDSL_CANNOT_HAPPEN;
        }
        memcpy(buf, cb, amount);
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
};

tdsl::uint8_t buf [512] = {0};

using uut_t             = tdsl::net::tdsl_netimpl_arduino<my_client>;

TEST(test, construct) {
    uut_t the_client{buf};
}

TEST(test, connect) {
    uut_t the_client{buf};
    auto r = the_client.connect(/*target=*/tdsl::string_view{"a"}, /*port=*/1555);
    (void) r;
}

TEST(test, write) {
    uut_t the_client{buf};
    the_client.do_write(tdsl::span<tdsl::uint8_t>{nullptr, nullptr});
}

TEST(test, receive_tds_pdu) {
    uut_t the_client{buf};
    the_client.do_receive_tds_pdu();
}

TEST(test, send_tds_pdu) {
    uut_t the_client{buf};
    the_client.do_send_tds_pdu(tdsl::detail::e_tds_message_type::login);
}