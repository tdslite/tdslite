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
#include <cstring>
#include <cstdint>

// PROGMEM mocks (program memory is regular memory on desktop)
#define PROGMEM
#define pgm_read_byte_near(x) *(x)

void * memcpy_P(void * __restrict dest, const void * __restrict src, size_t n) {
    return std::memcpy(dest, src, n);
}

std::function<void(int)> delay = [](int ms) {
    (void) ms;
};

// Mock steady clock. Stands still unless a test advances it. It's 32-bit,
// like millis() on Arduino.
std::uint32_t g_millis = 0;

unsigned long millis() {
    return g_millis;
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
    EXPECT_EQ(tdsl::detail::e_send_status::success,
              the_client.do_send_tds_pdu(tdsl::detail::e_tds_message_type::login));
}

// --------------------------------------------------------------------------------
// Receive path tests (network_io_base::do_receive_tds_pdu)
//
// These tests drive the real receive loop of network_io_base through the
// arduino network implementation, using a client that replays a scripted
// byte stream. They cover TDS packets whose data does not fit into the free
// space of the network buffer (either because the packet is larger than the
// buffer, or because unconsumed data from a previous packet occupies part of
// it), and receive failures.
// --------------------------------------------------------------------------------

#include <tdslite/detail/tdsl_command_context.hpp>

#include <algorithm>
#include <initializer_list>
#include <limits>
#include <vector>

namespace {

    using byte_vec                    = std::vector<tdsl::uint8_t>;

    constexpr tdsl::size_t k_no_limit = std::numeric_limits<tdsl::size_t>::max();

    /**
     * Arduino client that replays a scripted byte stream.
     *
     * Reading past the end of the stream, or reaching `fail_at`,
     * behaves like a disconnect (read() returns 0) so that a test
     * never hangs waiting for data that will not come.
     */
    struct scripted_client {
        scripted_client(const byte_vec & stream, tdsl::size_t max_read = k_no_limit,
                        tdsl::size_t fail_at = k_no_limit, byte_vec * tx = nullptr) :
            stream(stream), max_read(max_read), fail_at(fail_at), tx(tx) {}

        tdsl::size_t write(const unsigned char * data, tdsl::size_t len) {
            // Record the sent bytes, if requested
            if (tx && len) {
                tx->insert(tx->end(), data, data + len);
            }
            return len;
        }

        int connect(const char *, unsigned short) {
            return 1;
        }

        int read(unsigned char * dst, unsigned long amount) {
            const tdsl::size_t limit = fail_at < stream.size() ? fail_at : stream.size();
            if (pos >= limit) {
                return 0; // disconnected
            }
            tdsl::size_t n = amount;
            if (n > max_read) {
                n = max_read;
            }
            if (n > limit - pos) {
                n = limit - pos;
            }
            memcpy(dst, stream.data() + pos, n);
            pos += n;
            return static_cast<int>(n);
        }

        int available() {
            // Always report data so that read() gets the
            // chance to signal a disconnect at end of stream.
            return 1;
        }

        bool connected() {
            return true;
        }

        void flush() {}

        void stop() {}

        tdsl::uint16_t remotePort() {
            return 1433;
        }

        my_client::ip_addr remoteIP() {
            return my_client::ip_addr(127, 0, 0, 1);
        }

        byte_vec stream;
        tdsl::size_t max_read;
        tdsl::size_t fail_at;
        byte_vec * tx;
        tdsl::size_t pos = {0};
    };

    /**
     * Network implementation under test, with the network buffer exposed
     * so the tests can verify its state after an error.
     */
    struct recv_uut_t : public uut_t<scripted_client> {
        using uut_t<scripted_client>::uut_t;
        using uut_t<scripted_client>::network_buffer;
    };

    /**
     * Make a TDS packet (header + data)
     */
    inline byte_vec make_tds_packet(tdsl::uint8_t type, bool eom, const byte_vec & data) {
        const auto len = static_cast<tdsl::uint16_t>(data.size() + 8);
        byte_vec pkt{type,
                     static_cast<tdsl::uint8_t>(eom ? 0x01 : 0x00),
                     static_cast<tdsl::uint8_t>(len >> 8),
                     static_cast<tdsl::uint8_t>(len & 0xFF),
                     0x00,
                     0x00,
                     0x01,
                     0x00};
        pkt.insert(pkt.end(), data.begin(), data.end());
        return pkt;
    }

    /**
     * Deterministic test pattern
     */
    inline byte_vec make_pattern(tdsl::size_t size, tdsl::uint8_t seed = 0) {
        byte_vec v(size);
        for (tdsl::size_t i = 0; i < size; i++) {
            v [i] = static_cast<tdsl::uint8_t>((i * 7 + seed) & 0xFF);
        }
        return v;
    }

    inline byte_vec concat(std::initializer_list<byte_vec> parts) {
        byte_vec r;
        for (const auto & p : parts) {
            r.insert(r.end(), p.begin(), p.end());
        }
        return r;
    }

    /**
     * Packet data sink. Consumes data from the network buffer in
     * units of `record_size` bytes (i.e. like a token parser that
     * only consumes complete tokens) and records what it consumed.
     * A record_size of zero consumes nothing.
     */
    struct packet_data_sink {
        explicit packet_data_sink(tdsl::size_t record_size = 1) : record_size(record_size) {}

        static tdsl::uint32_t cb(void * uptr, tdsl::detail::e_tds_message_type,
                                 tdsl::binary_reader<tdsl::endian::little> & rdr) {
            auto & self = *static_cast<packet_data_sink *>(uptr);
            self.calls++;
            if (rdr.remaining_bytes() > self.max_seen) {
                self.max_seen = rdr.remaining_bytes();
            }
            if (0 == self.record_size) {
                return 1;
            }
            const auto consumable = (rdr.remaining_bytes() / self.record_size) * self.record_size;
            if (consumable) {
                const auto v = rdr.read(consumable);
                self.data.insert(self.data.end(), v.begin(), v.end());
            }
            return static_cast<tdsl::uint32_t>(rdr.remaining_bytes() ? self.record_size : 0);
        }

        tdsl::size_t record_size;
        tdsl::size_t calls    = {0};
        tdsl::size_t max_seen = {0};
        byte_vec data;
    };

    constexpr tdsl::uint8_t k_tabular_result = 0x04;

} // namespace

// A packet that is larger than the whole network buffer must be received in
// chunks and delivered completely, without tripping the buffer's in-use guard.
TEST(recv, oversized_packet_is_received_in_chunks) {
    tdsl::uint8_t nbuf [512] = {0};
    const auto data          = make_pattern(2000);
    recv_uut_t uut{nbuf, make_tds_packet(k_tabular_result, true, data)};
    packet_data_sink sink;
    uut.register_packet_data_callback(&packet_data_sink::cb, &sink);

    ASSERT_EQ(1u, uut.do_receive_tds_pdu());
    EXPECT_EQ(data, sink.data);
    EXPECT_GE(sink.calls, 4u);
    EXPECT_LE(sink.max_seen, sizeof(nbuf));
}

// Same as above, but the client delivers the data in small fragments.
TEST(recv, oversized_packet_with_fragmented_reads) {
    tdsl::uint8_t nbuf [512] = {0};
    const auto data          = make_pattern(2000);
    recv_uut_t uut{nbuf, make_tds_packet(k_tabular_result, true, data), /*max_read=*/7};
    packet_data_sink sink;
    uut.register_packet_data_callback(&packet_data_sink::cb, &sink);

    ASSERT_EQ(1u, uut.do_receive_tds_pdu());
    EXPECT_EQ(data, sink.data);
}

// The whole packet data must be pulled from the socket before the next
// TDS header is read. Otherwise, the tail of an oversized packet would be
// interpreted as the next header and the stream would get out of sync.
TEST(recv, oversized_packet_does_not_desync_stream) {
    tdsl::uint8_t nbuf [512] = {0};
    const auto p1            = make_pattern(2000, 1);
    const auto p2            = make_pattern(300, 2);
    const auto p3            = make_pattern(1500, 3);
    // Message #1 (two packets), followed by message #2 (one packet)
    recv_uut_t uut{nbuf, concat({make_tds_packet(k_tabular_result, false, p1),
                                 make_tds_packet(k_tabular_result, true, p2),
                                 make_tds_packet(k_tabular_result, true, p3)})};
    packet_data_sink sink;
    uut.register_packet_data_callback(&packet_data_sink::cb, &sink);

    ASSERT_EQ(2u, uut.do_receive_tds_pdu());
    EXPECT_EQ(concat({p1, p2}), sink.data);

    sink.data.clear();
    ASSERT_EQ(1u, uut.do_receive_tds_pdu());
    EXPECT_EQ(p3, sink.data);
}

// Packets that would fit into an empty buffer, but not into the free space
// left by unconsumed data (e.g. a token that spans packets). The unconsumed
// data must be kept and delivered in front of the new data.
TEST(recv, packet_larger_than_free_space_keeps_unconsumed_data) {
    tdsl::uint8_t nbuf [512] = {0};
    // 96-byte records; 500 is not a multiple of 96, so a partial
    // record is left over at the end of the first packet's data.
    const auto p1            = make_pattern(500, 4);
    const auto p2            = make_pattern(500, 5);
    recv_uut_t uut{nbuf, concat({make_tds_packet(k_tabular_result, false, p1),
                                 make_tds_packet(k_tabular_result, true, p2)})};
    packet_data_sink sink{/*record_size=*/96};
    uut.register_packet_data_callback(&packet_data_sink::cb, &sink);

    ASSERT_EQ(2u, uut.do_receive_tds_pdu());
    const auto all = concat({p1, p2});
    // 1000 bytes in total -> 10 complete records. The 40-byte
    // tail is discarded at the end of the message.
    ASSERT_EQ(960u, sink.data.size());
    EXPECT_TRUE(std::equal(sink.data.begin(), sink.data.end(), all.begin()));
    EXPECT_EQ(0u, uut.network_buffer.get_writer()->offset());
}

// The packet data fills the buffer exactly.
TEST(recv, packet_exactly_buffer_size) {
    tdsl::uint8_t nbuf [512] = {0};
    const auto data          = make_pattern(sizeof(nbuf));
    recv_uut_t uut{nbuf, make_tds_packet(k_tabular_result, true, data)};
    packet_data_sink sink;
    uut.register_packet_data_callback(&packet_data_sink::cb, &sink);

    ASSERT_EQ(1u, uut.do_receive_tds_pdu());
    EXPECT_EQ(data, sink.data);
    EXPECT_EQ(1u, sink.calls);
}

// A packet with no data must not invoke the callback.
TEST(recv, empty_packet) {
    tdsl::uint8_t nbuf [512] = {0};
    const auto p2            = make_pattern(10, 6);
    recv_uut_t uut{nbuf, concat({make_tds_packet(k_tabular_result, false, {}),
                                 make_tds_packet(k_tabular_result, true, p2)})};
    packet_data_sink sink;
    uut.register_packet_data_callback(&packet_data_sink::cb, &sink);

    ASSERT_EQ(2u, uut.do_receive_tds_pdu());
    EXPECT_EQ(p2, sink.data);
    EXPECT_EQ(1u, sink.calls);
}

// If the callback cannot consume anything from a full buffer (a single
// token larger than the network buffer), the receive must give up cleanly.
TEST(recv, buffer_exhausted_gives_up) {
    tdsl::uint8_t nbuf [512] = {0};
    recv_uut_t uut{nbuf, make_tds_packet(k_tabular_result, true, make_pattern(2000))};
    packet_data_sink sink{/*record_size=*/0};
    uut.register_packet_data_callback(&packet_data_sink::cb, &sink);

    ASSERT_EQ(0u, uut.do_receive_tds_pdu());
    EXPECT_EQ(1u, sink.calls);
    EXPECT_EQ(sizeof(nbuf), sink.max_seen);
    EXPECT_EQ(0u, uut.network_buffer.get_writer()->offset());
}

// Disconnect before / in the middle of the TDS header.
TEST(recv, header_receive_failure) {
    for (tdsl::size_t fail_at : {0, 3, 7}) {
        tdsl::uint8_t nbuf [512] = {0};
        recv_uut_t uut{nbuf, make_tds_packet(k_tabular_result, true, make_pattern(100)), k_no_limit,
                       fail_at};
        packet_data_sink sink;
        uut.register_packet_data_callback(&packet_data_sink::cb, &sink);

        EXPECT_EQ(0u, uut.do_receive_tds_pdu()) << "fail_at=" << fail_at;
        EXPECT_EQ(0u, sink.calls) << "fail_at=" << fail_at;
        EXPECT_EQ(0u, uut.network_buffer.get_writer()->offset()) << "fail_at=" << fail_at;
    }
}

// Disconnect in the middle of the packet data. The data received so far
// must not be passed down as if it was complete, and the buffer must be
// left empty.
TEST(recv, data_receive_failure) {
    // Fits-in-buffer case and oversized (chunked) case
    for (tdsl::size_t data_size : {300, 2000}) {
        tdsl::uint8_t nbuf [512] = {0};
        // header + 200 bytes of data, then disconnect
        recv_uut_t uut{nbuf, make_tds_packet(k_tabular_result, true, make_pattern(data_size)),
                       k_no_limit, /*fail_at=*/8 + 200};
        packet_data_sink sink{/*record_size=*/0};
        uut.register_packet_data_callback(&packet_data_sink::cb, &sink);

        EXPECT_EQ(0u, uut.do_receive_tds_pdu()) << "data_size=" << data_size;
        EXPECT_EQ(0u, sink.calls) << "data_size=" << data_size;
        EXPECT_EQ(0u, uut.network_buffer.get_writer()->offset()) << "data_size=" << data_size;
    }
}

// Disconnect in the middle of the second packet of a message.
TEST(recv, second_packet_receive_failure) {
    tdsl::uint8_t nbuf [512] = {0};
    const auto p1            = make_pattern(300, 7);
    const auto stream        = concat({make_tds_packet(k_tabular_result, false, p1),
                                       make_tds_packet(k_tabular_result, true, make_pattern(300))});
    recv_uut_t uut{nbuf, stream, k_no_limit, /*fail_at=*/stream.size() - 100};
    packet_data_sink sink;
    uut.register_packet_data_callback(&packet_data_sink::cb, &sink);

    EXPECT_EQ(1u, uut.do_receive_tds_pdu());
    EXPECT_EQ(p1, sink.data);
    EXPECT_EQ(0u, uut.network_buffer.get_writer()->offset());
}

// --------------------------------------------------------------------------------
// End-to-end: a result set larger than the network buffer, parsed by the
// real tds_context/command_context on top of the arduino network
// implementation. Rows straddle the chunk (and packet) boundaries.
// --------------------------------------------------------------------------------

namespace {

    using e2e_cmd_ctx_t =
        tdsl::detail::command_context<tdsl::net::tdsl_netimpl_arduino<scripted_client>>;
    using e2e_tds_ctx_t = e2e_cmd_ctx_t::tds_context_type;

    /**
     * Tabular result: COLMETADATA (one INT column), `row_count` ROW tokens
     * with values [0, row_count), and a final DONE token.
     */
    inline byte_vec make_int_result_set(tdsl::uint32_t row_count) {
        byte_vec v{
            0x81,       // COLMETADATA
            0x01, 0x00, // column count
            0x00, 0x00, // user type
            0x00, 0x00, // flags
            0x38,       // INT4TYPE
            0x01,       // column name length (chars)
            'n',  0x00  // column name
        };
        for (tdsl::uint32_t i = 0; i < row_count; i++) {
            v.push_back(0xD1); // ROW
            v.push_back(static_cast<tdsl::uint8_t>(i));
            v.push_back(static_cast<tdsl::uint8_t>(i >> 8));
            v.push_back(static_cast<tdsl::uint8_t>(i >> 16));
            v.push_back(static_cast<tdsl::uint8_t>(i >> 24));
        }
        const byte_vec done{0xFD, // DONE
                            0x10,
                            0x00, // status: DONE_COUNT
                            0xC1,
                            0x00, // curcmd: SELECT
                            static_cast<tdsl::uint8_t>(row_count),
                            static_cast<tdsl::uint8_t>(row_count >> 8),
                            static_cast<tdsl::uint8_t>(row_count >> 16),
                            static_cast<tdsl::uint8_t>(row_count >> 24)};
        v.insert(v.end(), done.begin(), done.end());
        return v;
    }

    /**
     * Split @p data into TDS packets with at most @p max_data bytes of data each
     */
    inline byte_vec packetize(const byte_vec & data, tdsl::size_t max_data) {
        byte_vec r;
        for (tdsl::size_t off = 0; off < data.size(); off += max_data) {
            const auto n   = (std::min) (data.size() - off, max_data);
            const auto beg = data.begin() + static_cast<std::ptrdiff_t>(off);
            const auto pkt = make_tds_packet(k_tabular_result, (off + n) == data.size(),
                                             byte_vec(beg, beg + static_cast<std::ptrdiff_t>(n)));
            r.insert(r.end(), pkt.begin(), pkt.end());
        }
        return r;
    }

    struct row_collector {
        static void cb(void * uptr, const tdsl::tds_colmetadata_token &,
                       const tdsl::tdsl_row & row) {
            auto & self = *static_cast<row_collector *>(uptr);
            if (row.size() != 1) {
                self.bad_rows++;
                return;
            }
            self.values.push_back(row [0].as<tdsl::int32_t>());
        }

        std::vector<tdsl::int32_t> values;
        tdsl::size_t bad_rows = {0};
    };

    void run_e2e(tdsl::uint32_t row_count, tdsl::size_t packet_data_size, tdsl::size_t max_read) {
        tdsl::uint8_t nbuf [512] = {0};
        const auto stream        = packetize(make_int_result_set(row_count), packet_data_size);
        e2e_tds_ctx_t tds_ctx{nbuf, stream, max_read};
        e2e_cmd_ctx_t cmd_ctx{tds_ctx};
        row_collector rc;

        const auto result =
            cmd_ctx.execute_query(tdsl::string_view{"SELECT n FROM t;"}, &row_collector::cb, &rc);

        EXPECT_EQ(0u, rc.bad_rows);
        ASSERT_EQ(row_count, rc.values.size());
        for (tdsl::uint32_t i = 0; i < row_count; i++) {
            ASSERT_EQ(static_cast<tdsl::int32_t>(i), rc.values [i]) << "row " << i;
        }
        EXPECT_EQ(row_count, result.affected_rows);
        EXPECT_TRUE(static_cast<bool>(result));
    }
} // namespace

// A single TDS packet (1520 bytes of data) through a 512-byte buffer
TEST(recv_e2e, result_set_in_single_oversized_packet) {
    run_e2e(/*row_count=*/300, /*packet_data_size=*/32000, k_no_limit);
}

// Multiple TDS packets, each larger than the 512-byte buffer
TEST(recv_e2e, result_set_in_multiple_oversized_packets) {
    run_e2e(/*row_count=*/1000, /*packet_data_size=*/1337, k_no_limit);
}

// Multiple TDS packets that fit into the buffer on their own, but not
// together with the partial row left over from the previous packet
TEST(recv_e2e, result_set_packets_larger_than_free_space) {
    run_e2e(/*row_count=*/1000, /*packet_data_size=*/509, k_no_limit);
}

// Same as above, with fragmented socket reads
TEST(recv_e2e, result_set_fragmented_reads) {
    run_e2e(/*row_count=*/1000, /*packet_data_size=*/509, /*max_read=*/13);
}

// --------------------------------------------------------------------------------
// Negotiated packet size
//
// The server confirms (or overrides) the requested packet size with an
// ENVCHANGE token in the login response. The packet size may be larger than
// the network buffer, and invalid values must not crash the client. The
// packet size in use is observed through the segmentation of a sent message.
// --------------------------------------------------------------------------------

#include <tdslite/detail/tdsl_driver.hpp>

namespace {

    /**
     * Lengths of the TDS packets in a sent byte stream
     */
    inline std::vector<tdsl::uint16_t> sent_packet_lengths(const byte_vec & tx) {
        std::vector<tdsl::uint16_t> r;
        tdsl::size_t off = 0;
        while (off + 8 <= tx.size()) {
            const auto len = static_cast<tdsl::uint16_t>((tx [off + 2] << 8) | tx [off + 3]);
            r.push_back(len);
            if (len < 8) {
                break;
            }
            off += len;
        }
        return r;
    }

    /**
     * Send a @p n byte message through @p net and return the
     * lengths of the TDS packets it is segmented into
     */
    template <typename NetT>
    inline std::vector<tdsl::uint16_t> send_message(NetT & net, byte_vec & tx, tdsl::size_t n) {
        tx.clear();
        const auto data = make_pattern(n);
        net.do_write(tdsl::byte_view{data.data(), static_cast<tdsl::uint32_t>(data.size())});
        EXPECT_EQ(tdsl::detail::e_send_status::success,
                  net.do_send_tds_pdu(tdsl::detail::e_tds_message_type::sql_batch));
        return sent_packet_lengths(tx);
    }

    using packet_lengths              = std::vector<tdsl::uint16_t>;

    // A 510-byte message is segmented as:
    // - packet size 512   -> 504 + 6 bytes of data (two packets)
    // - packet size >= 518 -> a single packet
    constexpr tdsl::size_t k_msg_size = 510;
    const packet_lengths k_segmented_as_512{512, 14};
    const packet_lengths k_single_packet{518};

    /**
     * UCS-2LE encode an ASCII string
     */
    inline byte_vec ucs2(const char * s) {
        byte_vec r;
        for (; *s; ++s) {
            r.push_back(static_cast<tdsl::uint8_t>(*s));
            r.push_back(0x00);
        }
        return r;
    }

    /**
     * ENVCHANGE (packet size) token with raw UCS-2 new/old values
     */
    inline byte_vec make_envchange_packet_size(const byte_vec & new_value,
                                               const byte_vec & old_value = ucs2("4096")) {
        byte_vec body{0x04 /*packet size*/, static_cast<tdsl::uint8_t>(new_value.size() / 2)};
        body.insert(body.end(), new_value.begin(), new_value.end());
        body.push_back(static_cast<tdsl::uint8_t>(old_value.size() / 2));
        body.insert(body.end(), old_value.begin(), old_value.end());
        byte_vec token{0xE3, static_cast<tdsl::uint8_t>(body.size() & 0xFF),
                       static_cast<tdsl::uint8_t>(body.size() >> 8)};
        token.insert(token.end(), body.begin(), body.end());
        return token;
    }

    inline byte_vec make_done_token(tdsl::uint32_t row_count = 0) {
        return byte_vec{0xFD,
                        static_cast<tdsl::uint8_t>(row_count ? 0x10 : 0x00),
                        0x00,
                        0xC1,
                        0x00,
                        static_cast<tdsl::uint8_t>(row_count),
                        static_cast<tdsl::uint8_t>(row_count >> 8),
                        static_cast<tdsl::uint8_t>(row_count >> 16),
                        static_cast<tdsl::uint8_t>(row_count >> 24)};
    }

    /**
     * Receive a response with the given ENVCHANGE (packet size) values through
     * a tds_context with a 512-byte network buffer, then send a message and
     * return its segmentation.
     */
    inline packet_lengths packet_lengths_after_envchange(std::initializer_list<byte_vec> values,
                                                         tdsl::size_t * envchange_count = nullptr) {
        byte_vec tokens;
        for (const auto & v : values) {
            const auto t = make_envchange_packet_size(v);
            tokens.insert(tokens.end(), t.begin(), t.end());
        }
        const auto done = make_done_token();
        tokens.insert(tokens.end(), done.begin(), done.end());

        tdsl::uint8_t nbuf [512] = {0};
        byte_vec tx;
        e2e_tds_ctx_t tds_ctx{nbuf, make_tds_packet(k_tabular_result, true, tokens), k_no_limit,
                              k_no_limit, &tx};
        tdsl::size_t count           = {0};
        tds_ctx.callbacks.envinfochg = {[](void * u, const tdsl::tds_envchange_token &) {
                                            ++*static_cast<tdsl::size_t *>(u);
                                        },
                                        &count};
        tds_ctx.receive_tds_pdu();
        if (envchange_count) {
            *envchange_count = count;
        }
        return send_message(tds_ctx, tx, k_msg_size);
    }

} // namespace

// The packet size must be settable to a value larger than the network buffer.
TEST(packet_size, larger_than_network_buffer_is_accepted) {
    tdsl::uint8_t nbuf [512] = {0};
    byte_vec tx;
    recv_uut_t uut{nbuf, byte_vec{}, k_no_limit, k_no_limit, &tx};

    uut.set_tds_packet_size(512);
    EXPECT_EQ(k_segmented_as_512, send_message(uut, tx, k_msg_size));

    uut.set_tds_packet_size(2048);
    EXPECT_EQ(k_single_packet, send_message(uut, tx, k_msg_size));

    uut.set_tds_packet_size(512);
    uut.set_tds_packet_size(32767);
    EXPECT_EQ(k_single_packet, send_message(uut, tx, k_msg_size));
}

// Out-of-range values must be ignored (e.g. a packet size <= 8 would make
// the message segment size zero or negative).
TEST(packet_size, out_of_range_values_are_ignored) {
    for (tdsl::uint16_t v : {0, 1, 7, 8, 9, 100, 511, 32768, 65535}) {
        tdsl::uint8_t nbuf [512] = {0};
        byte_vec tx;
        recv_uut_t uut{nbuf, byte_vec{}, k_no_limit, k_no_limit, &tx};
        uut.set_tds_packet_size(512);
        uut.set_tds_packet_size(v);
        EXPECT_EQ(k_segmented_as_512, send_message(uut, tx, k_msg_size)) << "value=" << v;
    }
}

// ENVCHANGE with a valid packet size is applied.
TEST(packet_size, envchange_valid_value_is_applied) {
    tdsl::size_t count = {0};
    EXPECT_EQ(k_segmented_as_512, packet_lengths_after_envchange({ucs2("512")}, &count));
    EXPECT_EQ(1u, count);
}

// ENVCHANGE with a packet size larger than the network buffer (512) is
// applied, without crashing the client.
TEST(packet_size, envchange_value_larger_than_network_buffer_is_applied) {
    EXPECT_EQ(k_segmented_as_512, packet_lengths_after_envchange({ucs2("512")}));
    EXPECT_EQ(k_single_packet, packet_lengths_after_envchange({ucs2("512"), ucs2("4096")}));
    EXPECT_EQ(k_single_packet, packet_lengths_after_envchange({ucs2("512"), ucs2("32767")}));
}

// ENVCHANGE with an invalid packet size is ignored. The previous (valid)
// packet size stays in effect, and the ENVCHANGE callback is still invoked.
TEST(packet_size, envchange_invalid_values_are_ignored) {
    const byte_vec invalid_values [] = {
        ucs2(""), // empty
        ucs2("abc"), // not a number
        ucs2("5 12"), // embedded space
        ucs2("-512"), // sign
        ucs2("8"), // too small (would make the segment size zero)
        ucs2("511"), // too small
        ucs2("32768"), // too large
        ucs2("70000"), // does not fit into 16 bits (used to wrap to 4464)
        ucs2("4294967808"), // does not fit into 32 bits (used to wrap to 512)
        byte_vec{'5', 0x01, '1', 0x00, '2', 0x00}, // non-ASCII code unit
    };
    for (const auto & v : invalid_values) {
        tdsl::size_t count = {0};
        EXPECT_EQ(k_segmented_as_512, packet_lengths_after_envchange({ucs2("512"), v}, &count))
            << "value bytes=" << v.size();
        EXPECT_EQ(2u, count);
    }
}

// --------------------------------------------------------------------------------

namespace {
    using driver_t = tdsl::detail::tdsl_driver<tdsl::net::tdsl_netimpl_arduino<scripted_client>>;

    inline byte_vec make_loginack_token() {
        const auto progname = ucs2("test");
        byte_vec body{0x01,                    // interface
                      0x71, 0x00, 0x00, 0x01}; // tds version
        body.push_back(static_cast<tdsl::uint8_t>(progname.size() / 2));
        body.insert(body.end(), progname.begin(), progname.end());
        body.insert(body.end(), {0x10, 0x00, 0x00, 0x00}); // program version
        byte_vec token{0xAD, static_cast<tdsl::uint8_t>(body.size() & 0xFF),
                       static_cast<tdsl::uint8_t>(body.size() >> 8)};
        token.insert(token.end(), body.begin(), body.end());
        return token;
    }
} // namespace

// The reported scenario: a network buffer smaller than the requested
// (default, 4096) packet size. The server confirms 4096 at login, which
// used to trap. Login must succeed, and a result set sent in 4096-byte
// packets must be received completely through the 512-byte buffer.
TEST(packet_size, driver_connect_with_packet_size_larger_than_buffer) {
    constexpr tdsl::uint32_t k_row_count = 1000;

    const auto login_response            = concat(
        {make_envchange_packet_size(ucs2("4096")), make_loginack_token(), make_done_token()});
    const auto stream        = concat({make_tds_packet(k_tabular_result, true, login_response),
                                       packetize(make_int_result_set(k_row_count), 4096 - 8)});

    tdsl::uint8_t nbuf [512] = {0};
    byte_vec tx;
    driver_t driver{nbuf, stream, k_no_limit, k_no_limit, &tx};

    driver_t::connection_parameters params;
    params.server_name = "server";
    params.user_name   = "user";
    params.password    = "pass";
    ASSERT_EQ(4096u, params.packet_size); // the default
    ASSERT_EQ(driver_t::e_driver_error_code::success, driver.connect(params));

    row_collector rc;
    const auto result = driver.execute_query("SELECT n FROM t;", &row_collector::cb, &rc);
    EXPECT_EQ(0u, rc.bad_rows);
    ASSERT_EQ(k_row_count, rc.values.size());
    for (tdsl::uint32_t i = 0; i < k_row_count; i++) {
        ASSERT_EQ(static_cast<tdsl::int32_t>(i), rc.values [i]) << "row " << i;
    }
    EXPECT_EQ(k_row_count, result.affected_rows);
}

// --------------------------------------------------------------------------------
// Requests larger than the network buffer
//
// A request that does not fit into the network buffer must not be sent at
// all. Sending the part that fit would execute a truncated statement, e.g.
// an UPDATE without its WHERE clause.
// --------------------------------------------------------------------------------

namespace {

    /**
     * An UPDATE statement whose WHERE clause starts after @p pad_chars
     * characters of padding (each character is 2 bytes on the wire)
     */
    inline std::string make_update_stmt(tdsl::size_t pad_chars) {
        return "UPDATE t SET v = 1 /*" + std::string(pad_chars, '.') + "*/ WHERE id = 42;";
    }

    inline tdsl::string_view as_sv(const std::string & s) {
        return tdsl::string_view{s.data(), static_cast<tdsl::uint32_t>(s.size())};
    }

    /**
     * Response to a single-row, single-INT-column query (value 0)
     */
    inline byte_vec make_small_query_response() {
        return make_tds_packet(k_tabular_result, true, make_int_result_set(1));
    }
} // namespace

// Network layer: a message that overflows the buffer is discarded, nothing
// is sent, and the next message is not affected.
TEST(request_too_large, network_layer_discards_overflowed_message) {
    tdsl::uint8_t nbuf [512] = {0};
    byte_vec tx;
    recv_uut_t uut{nbuf, byte_vec{}, k_no_limit, k_no_limit, &tx};

    // Too large, written in small pieces (like a string)
    const auto big = make_pattern(600);
    for (tdsl::size_t i = 0; i < big.size(); i += 2) {
        uut.do_write(tdsl::byte_view{&big [i], 2});
    }
    EXPECT_EQ(tdsl::detail::e_send_status::request_too_large,
              uut.do_send_tds_pdu(tdsl::detail::e_tds_message_type::sql_batch));
    EXPECT_TRUE(tx.empty());
    EXPECT_EQ(0u, uut.network_buffer.get_writer()->offset());

    // Too large, in a single write
    uut.do_write(tdsl::byte_view{big.data(), static_cast<tdsl::uint32_t>(big.size())});
    EXPECT_EQ(tdsl::detail::e_send_status::request_too_large,
              uut.do_send_tds_pdu(tdsl::detail::e_tds_message_type::sql_batch));
    EXPECT_TRUE(tx.empty());

    // Out-of-bounds write at an offset (e.g. a placeholder)
    uut.do_write(tdsl::byte_view{big.data(), 10});
    uut.do_write(/*offset=*/600, tdsl::byte_view{big.data(), 2});
    EXPECT_EQ(tdsl::detail::e_send_status::request_too_large,
              uut.do_send_tds_pdu(tdsl::detail::e_tds_message_type::sql_batch));
    EXPECT_TRUE(tx.empty());

    // The overflow state does not leak into the next message
    EXPECT_EQ(k_single_packet, send_message(uut, tx, k_msg_size));

    // A message that fills the buffer exactly is sent
    EXPECT_EQ(packet_lengths{512 + 8}, send_message(uut, tx, sizeof(nbuf)));
}

// The motivating case: the WHERE clause of an UPDATE does not fit into the
// buffer. Nothing must be sent, the query must report the failure, and the
// connection must stay usable.
TEST(request_too_large, execute_query_sends_nothing) {
    tdsl::uint8_t nbuf [512] = {0};
    byte_vec tx;
    // The stream only holds the response to the second (small) query.
    // If the failed query tried to receive a response, it would consume it.
    e2e_tds_ctx_t tds_ctx{nbuf, make_small_query_response(), k_no_limit, k_no_limit, &tx};
    e2e_cmd_ctx_t cmd_ctx{tds_ctx};

    // "UPDATE t SET v = 1 /*" is 21 chars (42 bytes), the WHERE
    // clause starts at byte 42 + 2 * (pad + 2) > 512.
    const auto stmt = make_update_stmt(/*pad_chars=*/250);
    ASSERT_GT(stmt.size() * 2, sizeof(nbuf));

    const auto result = cmd_ctx.execute_query(as_sv(stmt));
    EXPECT_TRUE(result.request_too_large);
    EXPECT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(0u, result.affected_rows);
    EXPECT_TRUE(tx.empty()) << "a truncated statement was sent";

    row_collector rc;
    const auto result2 =
        cmd_ctx.execute_query(tdsl::string_view{"SELECT 0;"}, &row_collector::cb, &rc);
    EXPECT_FALSE(result2.request_too_large);
    EXPECT_TRUE(static_cast<bool>(result2));
    EXPECT_FALSE(tx.empty());
    ASSERT_EQ(1u, rc.values.size());
    EXPECT_EQ(0, rc.values [0]);
}

// Same for wide-character (UTF-16) queries, which are written in one piece.
TEST(request_too_large, execute_query_wide_sends_nothing) {
    tdsl::uint8_t nbuf [512] = {0};
    byte_vec tx;
    e2e_tds_ctx_t tds_ctx{nbuf, byte_vec{}, k_no_limit, k_no_limit, &tx};
    e2e_cmd_ctx_t cmd_ctx{tds_ctx};

    const std::u16string stmt(300, u'x');
    const auto result = cmd_ctx.execute_query(
        tdsl::wstring_view{stmt.data(), static_cast<tdsl::uint32_t>(stmt.size())});
    EXPECT_TRUE(result.request_too_large);
    EXPECT_FALSE(static_cast<bool>(result));
    EXPECT_TRUE(tx.empty());
}

// A query that fills the buffer exactly is still sent.
TEST(request_too_large, execute_query_exactly_buffer_size_is_sent) {
    tdsl::uint8_t nbuf [512] = {0};
    byte_vec tx;
    e2e_tds_ctx_t tds_ctx{nbuf, make_small_query_response(), k_no_limit, k_no_limit, &tx};
    e2e_cmd_ctx_t cmd_ctx{tds_ctx};

    const std::string stmt(sizeof(nbuf) / 2, ' ');
    const auto result = cmd_ctx.execute_query(as_sv(stmt));
    EXPECT_FALSE(result.request_too_large);
    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_EQ(packet_lengths{512 + 8}, sent_packet_lengths(tx));
}

// RPC: a parameter value that does not fit used to be dropped while its
// length prefix was still sent (malformed request).
TEST(request_too_large, execute_rpc_sends_nothing) {
    tdsl::uint8_t nbuf [512] = {0};
    byte_vec tx;
    e2e_tds_ctx_t tds_ctx{nbuf, make_small_query_response(), k_no_limit, k_no_limit, &tx};
    e2e_cmd_ctx_t cmd_ctx{tds_ctx};

    const auto big = make_pattern(600);
    tdsl::detail::sql_parameter_varbinary p0{};
    p0 = tdsl::byte_view{big.data(), static_cast<tdsl::uint32_t>(big.size())};
    tdsl::detail::sql_parameter_binding params [] = {p0};

    const auto result = cmd_ctx.execute_rpc(tdsl::string_view{"SELECT @p0"}, params);
    ASSERT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(tdsl::detail::e_rpc_error_code::request_too_large, result.error());
    EXPECT_TRUE(tx.empty());

    // The connection is still usable
    row_collector rc;
    const auto result2 =
        cmd_ctx.execute_query(tdsl::string_view{"SELECT 0;"}, &row_collector::cb, &rc);
    EXPECT_TRUE(static_cast<bool>(result2));
    EXPECT_EQ(1u, rc.values.size());
}

// Login: a login request that does not fit is not sent, and connect()
// reports the failure.
TEST(request_too_large, login_sends_nothing) {
    tdsl::uint8_t nbuf [512] = {0};
    byte_vec tx;
    driver_t driver{nbuf, byte_vec{}, k_no_limit, k_no_limit, &tx};

    const std::string long_app_name(300, 'a');
    driver_t::connection_parameters params;
    params.server_name = "server";
    params.user_name   = "user";
    params.password    = "pass";
    params.app_name    = as_sv(long_app_name);
    EXPECT_EQ(driver_t::e_driver_error_code::login_failed, driver.connect(params));
    EXPECT_TRUE(tx.empty());
}

// --------------------------------------------------------------------------------
// Malformed and fragmented responses
//
// Malformed tokens must not crash the client (binary_reader traps on
// out-of-bounds reads, also in release builds) or make it parse token data as
// tokens. Tokens split across packets must be parsed correctly. The
// connection must stay usable for the next query either way.
// --------------------------------------------------------------------------------

namespace {

    using query_result_t = e2e_cmd_ctx_t::query_result;

    /**
     * Token with a 16-bit length field
     */
    inline byte_vec make_token(tdsl::uint8_t type, const byte_vec & body) {
        byte_vec r{type, static_cast<tdsl::uint8_t>(body.size() & 0xFF),
                   static_cast<tdsl::uint8_t>(body.size() >> 8)};
        r.insert(r.end(), body.begin(), body.end());
        return r;
    }

    /**
     * INFO token body (TDS 7.1 layout)
     */
    inline byte_vec make_info_body(const char * msg, const char * server, const char * proc,
                                   tdsl::uint16_t line = 1) {
        const auto m = ucs2(msg), s = ucs2(server), p = ucs2(proc);
        byte_vec b{0x39, 0x30, 0x00, 0x00, // number (12345)
                   0x01,                   // state
                   0x0A};                  // class
        b.push_back(static_cast<tdsl::uint8_t>((m.size() / 2) & 0xFF));
        b.push_back(static_cast<tdsl::uint8_t>((m.size() / 2) >> 8));
        b.insert(b.end(), m.begin(), m.end());
        b.push_back(static_cast<tdsl::uint8_t>(s.size() / 2));
        b.insert(b.end(), s.begin(), s.end());
        b.push_back(static_cast<tdsl::uint8_t>(p.size() / 2));
        b.insert(b.end(), p.begin(), p.end());
        b.push_back(static_cast<tdsl::uint8_t>(line & 0xFF));
        b.push_back(static_cast<tdsl::uint8_t>(line >> 8));
        return b;
    }

    struct info_counter {
        static void cb(void * uptr, const tdsl::tds_info_token &) {
            ++*static_cast<tdsl::size_t *>(uptr);
        }
    };

    /**
     * Collects rows of INT columns of any count
     */
    struct int_rows_collector {
        static void cb(void * uptr, const tdsl::tds_colmetadata_token &,
                       const tdsl::tdsl_row & row) {
            auto & self = *static_cast<int_rows_collector *>(uptr);
            std::vector<tdsl::int32_t> r;
            for (tdsl::size_t i = 0; i < row.size(); i++) {
                r.push_back(row [i].as<tdsl::int32_t>());
            }
            self.rows.push_back(r);
        }

        std::vector<std::vector<tdsl::int32_t>> rows;
    };

    /**
     * Result set with two INT columns and @p row_count rows ({i, -i})
     */
    inline byte_vec make_two_int_col_result_set(tdsl::uint32_t row_count) {
        byte_vec v{0x81,                                            // COLMETADATA
                   0x02, 0x00,                                      // column count
                   0x00, 0x00, 0x00, 0x00, 0x38, 0x01, 0x61, 0x00,  // INT4 "a"
                   0x00, 0x00, 0x00, 0x00, 0x38, 0x01, 0x62, 0x00}; // INT4 "b"
        for (tdsl::uint32_t i = 0; i < row_count; i++) {
            const auto neg = static_cast<tdsl::uint32_t>(-static_cast<tdsl::int32_t>(i));
            v.push_back(0xD1); // ROW
            for (const auto x : {i, neg}) {
                for (int s = 0; s < 32; s += 8) {
                    v.push_back(static_cast<tdsl::uint8_t>(x >> s));
                }
            }
        }
        const auto done = make_done_token(row_count);
        v.insert(v.end(), done.begin(), done.end());
        return v;
    }

    /**
     * Result set with a single TEXT column (has collation and table name in
     * its metadata) and no rows
     */
    inline byte_vec make_text_col_empty_result_set() {
        byte_vec v{0x81,                         // COLMETADATA
                   0x01, 0x00,                   // column count
                   0x00, 0x00,                   // user type
                   0x00, 0x00,                   // flags
                   0x23,                         // TEXTTYPE
                   0xFF, 0xFF, 0xFF, 0x7F,       // max length
                   0x09, 0x04, 0xD0, 0x00, 0x34, // collation
                   0x05, 0x00};                  // table name length (chars)
        const auto tname = ucs2("dbo.t");
        v.insert(v.end(), tname.begin(), tname.end());
        const auto cname = ucs2("c");
        v.push_back(0x01); // column name length (chars)
        v.insert(v.end(), cname.begin(), cname.end());
        const auto done = make_done_token(0);
        v.insert(v.end(), done.begin(), done.end());
        return v;
    }

    /**
     * Response of the follow-up query used for checking that the
     * connection is still usable: 3 rows, values {0, 1, 2}
     */
    inline byte_vec make_followup_response() {
        return make_tds_packet(k_tabular_result, true, make_int_result_set(3));
    }

    inline void expect_followup_query_works(e2e_cmd_ctx_t & cmd_ctx) {
        row_collector rc;
        const auto r =
            cmd_ctx.execute_query(tdsl::string_view{"SELECT n FROM t;"}, &row_collector::cb, &rc);
        EXPECT_TRUE(static_cast<bool>(r));
        EXPECT_FALSE(r.response_error);
        EXPECT_EQ(3u, r.affected_rows);
        EXPECT_EQ((std::vector<tdsl::int32_t>{0, 1, 2}), rc.values);
    }

    /**
     * Run a query that receives @p stream (TDS packets), then the
     * follow-up query
     */
    template <typename Collector>
    inline query_result_t run_query_stream(const byte_vec & stream, Collector & collector,
                                           tdsl::size_t * info_count = nullptr) {
        tdsl::uint8_t nbuf [512] = {0};
        e2e_tds_ctx_t tds_ctx{nbuf, concat({stream, make_followup_response()})};
        e2e_cmd_ctx_t cmd_ctx{tds_ctx};
        if (info_count) {
            tds_ctx.callbacks.info = {&info_counter::cb, info_count};
        }
        const auto r =
            cmd_ctx.execute_query(tdsl::string_view{"SELECT 1;"}, &Collector::cb, &collector);
        expect_followup_query_works(cmd_ctx);
        return r;
    }

    /**
     * Same as above, with @p response in a single packet
     */
    template <typename Collector>
    inline query_result_t run_query(const byte_vec & response, Collector & collector,
                                    tdsl::size_t * info_count = nullptr) {
        return run_query_stream(make_tds_packet(k_tabular_result, true, response), collector,
                                info_count);
    }

    /**
     * @p data split into two packets at @p split
     */
    inline byte_vec split_into_two_packets(const byte_vec & data, tdsl::size_t split) {
        const auto split_at = data.begin() + static_cast<std::ptrdiff_t>(split);
        return concat({make_tds_packet(k_tabular_result, false, byte_vec(data.begin(), split_at)),
                       make_tds_packet(k_tabular_result, true, byte_vec(split_at, data.end()))});
    }
} // namespace

// An ENVCHANGE token with no data used to trap on reading its type
TEST(malformed_response, envchange_with_zero_length_is_skipped) {
    row_collector rc;
    const auto r = run_query(concat({
                                 {0xE3, 0x00, 0x00},
                                 make_int_result_set(2)
    }),
                             rc);
    EXPECT_TRUE(r.response_error);
    EXPECT_FALSE(static_cast<bool>(r));
    // The rest of the response is still processed
    EXPECT_EQ((std::vector<tdsl::int32_t>{0, 1}), rc.values);
    EXPECT_EQ(2u, r.affected_rows);
}

// INFO tokens cut short at every possible position. Some positions used to
// trap (reading a string length or the line number past the end of the
// token), others made the parser lose track of the token boundaries.
TEST(malformed_response, info_token_truncated_at_every_position_is_skipped) {
    const auto body = make_info_body("message", "server", "proc");
    for (tdsl::size_t keep = 0; keep < body.size(); keep++) {
        SCOPED_TRACE(keep);
        tdsl::size_t infos = 0;
        row_collector rc;
        const auto truncated =
            byte_vec(body.begin(), body.begin() + static_cast<std::ptrdiff_t>(keep));
        const auto r = run_query(
            concat({make_token(0xAB, truncated), make_token(0xAB, body), make_int_result_set(1)}),
            rc, &infos);
        EXPECT_TRUE(r.response_error);
        EXPECT_EQ(1u, infos) << "the valid INFO token after the malformed one is lost";
        EXPECT_EQ((std::vector<tdsl::int32_t>{0}), rc.values);
    }
}

// The smallest valid INFO token (all strings empty) is 12 bytes in TDS 7.1
TEST(malformed_response, info_token_with_empty_strings_is_valid) {
    tdsl::size_t infos = 0;
    row_collector rc;
    const auto r = run_query(
        concat({make_token(0xAB, make_info_body("", "", "")), make_int_result_set(1)}), rc, &infos);
    EXPECT_FALSE(r.response_error);
    EXPECT_TRUE(static_cast<bool>(r));
    EXPECT_EQ(1u, infos);
    EXPECT_EQ((std::vector<tdsl::int32_t>{0}), rc.values);
}

// A result set split into two packets at every possible position. A
// COLMETADATA token cut after its first column used to be accepted with
// the remaining column(s) missing, and the rest of it parsed as tokens.
TEST(malformed_response, result_set_split_at_every_position) {
    const auto rs = make_two_int_col_result_set(3);
    const std::vector<std::vector<tdsl::int32_t>> expected{
        {0, 0 },
        {1, -1},
        {2, -2}
    };
    for (tdsl::size_t split = 1; split < rs.size(); split++) {
        SCOPED_TRACE(split);
        int_rows_collector c;
        const auto r = run_query_stream(split_into_two_packets(rs, split), c);
        EXPECT_FALSE(r.response_error);
        EXPECT_TRUE(static_cast<bool>(r));
        EXPECT_EQ(3u, r.affected_rows);
        EXPECT_EQ(expected, c.rows);
    }
}

// Same for column metadata with collation and table name (TEXT). Some split
// positions used to trap on reading the column name length.
TEST(malformed_response, text_column_metadata_split_at_every_position) {
    const auto rs = make_text_col_empty_result_set();
    for (tdsl::size_t split = 1; split < rs.size(); split++) {
        SCOPED_TRACE(split);
        int_rows_collector c;
        const auto r = run_query_stream(split_into_two_packets(rs, split), c);
        EXPECT_FALSE(r.response_error);
        EXPECT_TRUE(static_cast<bool>(r));
        EXPECT_TRUE(c.rows.empty());
    }
}

// A column type the client can't parse. The position of the next token is
// unknown, so the rest of the response must be discarded instead of being
// parsed as tokens.
TEST(malformed_response, unsupported_column_type_discards_response) {
    const byte_vec rs = concat({
        {0x81, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
         0xF1, // XML, not supported
         0x00, 0x01, 0x78, 0x00},
        {0xD1, 0x01, 0x02, 0x03, 0x04},
        make_done_token(1)
    });
    int_rows_collector c;
    const auto r = run_query(rs, c);
    EXPECT_TRUE(r.response_error);
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_TRUE(c.rows.empty());
}

// ROW without a prior COLMETADATA
TEST(malformed_response, row_without_colmetadata_discards_response) {
    int_rows_collector c;
    const auto r = run_query(concat({
                                 {0xD1, 0x01, 0x02, 0x03, 0x04},
                                 make_done_token(1)
    }),
                             c);
    EXPECT_TRUE(r.response_error);
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_TRUE(c.rows.empty());
}

// COLMETADATA with column count 0xFFFF ("no metadata") is valid and
// has nothing after the count
TEST(malformed_response, colmetadata_with_no_metadata_is_valid) {
    int_rows_collector c;
    const auto r = run_query(concat({
                                 {0x81, 0xFF, 0xFF},
                                 make_done_token(0)
    }),
                             c);
    EXPECT_FALSE(r.response_error);
    EXPECT_TRUE(static_cast<bool>(r));
    EXPECT_TRUE(c.rows.empty());
}

// execute_rpc reports a response that couldn't be processed
TEST(malformed_response, execute_rpc_reports_invalid_response) {
    tdsl::uint8_t nbuf [512] = {0};
    e2e_tds_ctx_t tds_ctx{
        nbuf, concat({make_tds_packet(k_tabular_result, true,
                      concat({{0xD1, 0x01, 0x02, 0x03, 0x04}, make_done_token(1)})),
                      make_followup_response()}
         )
    };
    e2e_cmd_ctx_t cmd_ctx{tds_ctx};
    tdsl::detail::sql_parameter_int p0{};
    tdsl::detail::sql_parameter_binding params [] = {p0};
    const auto r = cmd_ctx.execute_rpc(tdsl::string_view{"SELECT @p0"}, params);
    ASSERT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(tdsl::detail::e_rpc_error_code::invalid_response, r.error());
    expect_followup_query_works(cmd_ctx);
}

// --------------------------------------------------------------------------------
// Server errors
//
// SQL Server reports a failed statement with the error bits in the status of
// its DONE token (and an ERROR token with the message). The error must be
// reported to the caller, also when a later DONE token in the same response
// has no error bits.
// --------------------------------------------------------------------------------

namespace {

    constexpr tdsl::uint8_t k_done       = 0xFD;
    constexpr tdsl::uint8_t k_doneproc   = 0xFE;
    constexpr tdsl::uint8_t k_doneinproc = 0xFF;

    constexpr tdsl::uint16_t k_more      = 0x01;
    constexpr tdsl::uint16_t k_error     = 0x02;
    constexpr tdsl::uint16_t k_count     = 0x10;
    constexpr tdsl::uint16_t k_srverror  = 0x100;

    inline byte_vec make_done(tdsl::uint8_t type, tdsl::uint16_t status,
                              tdsl::uint32_t row_count = 0) {
        return byte_vec{type,
                        static_cast<tdsl::uint8_t>(status & 0xFF),
                        static_cast<tdsl::uint8_t>(status >> 8),
                        0xC3, // curcmd: INSERT
                        0x00,
                        static_cast<tdsl::uint8_t>(row_count),
                        static_cast<tdsl::uint8_t>(row_count >> 8),
                        static_cast<tdsl::uint8_t>(row_count >> 16),
                        static_cast<tdsl::uint8_t>(row_count >> 24)};
    }

    inline byte_vec make_error_token() {
        return make_token(0xAA, make_info_body("Violation of PRIMARY KEY constraint", "srv", ""));
    }

    /**
     * Run an RPC that receives @p response, then the follow-up query
     */
    inline e2e_cmd_ctx_t::execute_rpc_result run_rpc(const byte_vec & response,
                                                     tdsl::size_t * info_count = nullptr) {
        tdsl::uint8_t nbuf [512] = {0};
        e2e_tds_ctx_t tds_ctx{nbuf, concat({make_tds_packet(k_tabular_result, true, response),
                                            make_followup_response()})};
        e2e_cmd_ctx_t cmd_ctx{tds_ctx};
        if (info_count) {
            tds_ctx.callbacks.info = {&info_counter::cb, info_count};
        }
        tdsl::detail::sql_parameter_int p0{};
        tdsl::detail::sql_parameter_binding params [] = {p0};
        const auto r = cmd_ctx.execute_rpc(tdsl::string_view{"INSERT INTO t VALUES(@p0)"}, params);
        // The error must not leak into the next command
        expect_followup_query_works(cmd_ctx);
        return r;
    }
} // namespace

// The error used to be dropped, and the RPC reported 0 affected rows
TEST(server_error, execute_rpc_reports_server_error) {
    tdsl::size_t infos = 0;
    const auto r       = run_rpc(concat({make_error_token(), make_done(k_doneinproc, k_error),
                                         make_done(k_doneproc, k_error)}),
                                 &infos);
    ASSERT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(tdsl::detail::e_rpc_error_code::server_error, r.error());
    // The error message is still delivered to the info callback
    EXPECT_EQ(1u, infos);
}

// The error bit is only in the DONEINPROC of the failed statement, the
// final DONEPROC has none
TEST(server_error, execute_rpc_error_in_earlier_done_token_is_kept) {
    const auto r = run_rpc(concat(
        {make_error_token(), make_done(k_doneinproc, k_more | k_error), make_done(k_doneproc, 0)}));
    ASSERT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(tdsl::detail::e_rpc_error_code::server_error, r.error());
}

TEST(server_error, execute_rpc_reports_srverror) {
    const auto r = run_rpc(make_done(k_doneproc, k_srverror));
    ASSERT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(tdsl::detail::e_rpc_error_code::server_error, r.error());
}

TEST(server_error, execute_rpc_success_returns_affected_rows) {
    auto r =
        run_rpc(concat({make_done(k_doneinproc, k_more | k_count, 5), make_done(k_doneproc, 0)}));
    ASSERT_TRUE(static_cast<bool>(r));
    EXPECT_EQ(5u, *r);
}

// A batch whose first statement fails and second succeeds used to be
// reported as successful
TEST(server_error, execute_query_error_in_earlier_statement_is_kept) {
    row_collector rc;
    const auto r = run_query(
        concat({make_error_token(), make_done(k_done, k_more | k_error), make_int_result_set(1)}),
        rc);
    EXPECT_FALSE(static_cast<bool>(r));
    EXPECT_TRUE(r.status.error());
    // The rest of the batch is still processed
    EXPECT_EQ((std::vector<tdsl::int32_t>{0}), rc.values);
    EXPECT_EQ(1u, r.affected_rows);
}

// --------------------------------------------------------------------------------
// Host name passed to Client::connect
//
// The host name is copied into the network buffer (it may be a PROGMEM
// string) and passed to `connect(const char*, port)`, which expects a
// NUL-terminated string. The network buffer may contain stale data.
// --------------------------------------------------------------------------------

namespace {

    struct host_recording_client : public my_client {
        int connect(const char * host, unsigned short) {
            // Reads up to the NUL terminator, like a real Client
            last_host = host;
            ++calls;
            return 1;
        }

        static std::string last_host;
        static int calls;
    };

    std::string host_recording_client::last_host;
    int host_recording_client::calls = 0;

    struct host_uut_t : public uut_t<host_recording_client> {
        using uut_t<host_recording_client>::uut_t;
        using uut_t<host_recording_client>::network_buffer;
    };

    const char PROGMEM k_progmem_host [] = "progmem.host";
} // namespace

TEST(connect_host, host_is_nul_terminated) {
    tdsl::uint8_t nbuf [64];
    std::memset(nbuf, 0xAA, sizeof(nbuf)); // stale data
    host_uut_t uut{nbuf};
    host_recording_client::calls = 0;
    ASSERT_TRUE(static_cast<bool>(uut.connect(tdsl::string_view{"db.local"}, 1433)));
    EXPECT_EQ(1, host_recording_client::calls);
    EXPECT_EQ("db.local", host_recording_client::last_host);
}

TEST(connect_host, progmem_host_is_nul_terminated) {
    tdsl::uint8_t nbuf [64];
    std::memset(nbuf, 0xAA, sizeof(nbuf));
    host_uut_t uut{nbuf};
    ASSERT_TRUE(static_cast<bool>(uut.connect(tdsl::progmem_string_view{k_progmem_host}, 1433)));
    EXPECT_EQ("progmem.host", host_recording_client::last_host);
}

// Stale data from an earlier message must not leak into the host name
TEST(connect_host, host_is_nul_terminated_after_discarded_message) {
    tdsl::uint8_t nbuf [64] = {0};
    host_uut_t uut{nbuf};
    const auto junk = make_pattern(40, 0x41);
    uut.do_write(tdsl::byte_view{junk.data(), static_cast<tdsl::uint32_t>(junk.size())});
    uut.network_buffer.get_writer()->reset(); // like a discarded message
    ASSERT_TRUE(static_cast<bool>(uut.connect(tdsl::string_view{"db"}, 1433)));
    EXPECT_EQ("db", host_recording_client::last_host);
}

// A host name that fills the buffer leaves no space for the NUL terminator
TEST(connect_host, host_filling_the_buffer_is_rejected) {
    tdsl::uint8_t nbuf [8];
    std::memset(nbuf, 0xAA, sizeof(nbuf));
    host_uut_t uut{nbuf};
    host_recording_client::calls = 0;
    EXPECT_FALSE(static_cast<bool>(uut.connect(tdsl::string_view{"12345678"}, 1433)));
    EXPECT_EQ(0, host_recording_client::calls);

    // One character less fits
    ASSERT_TRUE(static_cast<bool>(uut.connect(tdsl::string_view{"1234567"}, 1433)));
    EXPECT_EQ(1, host_recording_client::calls);
    EXPECT_EQ("1234567", host_recording_client::last_host);
}

// --------------------------------------------------------------------------------
// Receive timeout
//
// do_recv() gives up after 30 seconds without completing. millis() is a
// 32-bit counter on Arduino, which wraps around every ~49.7 days.
// --------------------------------------------------------------------------------

namespace {

    constexpr tdsl::uint32_t k_recv_timeout_ms  = 30000;
    constexpr tdsl::uint32_t k_poll_interval_ms = 300;
    constexpr int k_errc_timeout                = -2;

    /**
     * Client whose data becomes available after @p polls_until_ready calls to
     * available() returned 0. Reading advances the clock by @p read_ms.
     */
    struct slow_client : public my_client {
        int available() {
            if (polls_until_ready > 0) {
                --polls_until_ready;
                return 0;
            }
            return 1;
        }

        int read(unsigned char * buf, unsigned long amount) {
            g_millis += read_ms;
            std::memset(buf, 0x5A, amount);
            return static_cast<int>(amount);
        }

        static int polls_until_ready;
        static tdsl::uint32_t read_ms;
    };

    int slow_client::polls_until_ready  = 0;
    tdsl::uint32_t slow_client::read_ms = 0;

    /**
     * Sets the mock clock and makes delay() advance it, for the scope
     */
    struct mock_clock_scope {
        explicit mock_clock_scope(tdsl::uint32_t now) : saved_delay(delay) {
            g_millis = now;
            delays   = 0;
            delay    = [](int ms) {
                g_millis += static_cast<tdsl::uint32_t>(ms);
                ++delays;
            };
        }

        ~mock_clock_scope() {
            delay    = saved_delay;
            g_millis = 0;
        }

        std::function<void(int)> saved_delay;
        static int delays;
    };

    int mock_clock_scope::delays = 0;

    /**
     * Receive 16 bytes from a slow_client, starting at time @p now
     */
    inline uut_t<slow_client>::network_io_result
    recv_from_slow_client(tdsl::uint32_t now, int polls_until_ready, tdsl::uint32_t read_ms = 0) {
        mock_clock_scope clock{now};
        slow_client::polls_until_ready = polls_until_ready;
        slow_client::read_ms           = read_ms;
        tdsl::uint8_t nbuf [64]        = {0};
        uut_t<slow_client> uut{nbuf};
        tdsl::uint8_t dst [16] = {0};
        return uut.do_recv(sizeof(dst), tdsl::byte_span{dst});
    }
} // namespace

TEST(recv_timeout, data_arriving_within_timeout_is_received) {
    EXPECT_TRUE(static_cast<bool>(recv_from_slow_client(/*now=*/5000, /*polls_until_ready=*/5)));
}

// The receive starts 1 second before millis() wraps around. It used to time
// out immediately (the deadline wrapped around to a small value) and lose
// the data it had read.
TEST(recv_timeout, receive_across_millis_wraparound) {
    EXPECT_TRUE(static_cast<bool>(
        recv_from_slow_client(/*now=*/0xFFFFFFFFu - 1000u, /*polls_until_ready=*/0)));
    EXPECT_TRUE(static_cast<bool>(
        recv_from_slow_client(/*now=*/0xFFFFFFFFu - 1000u, /*polls_until_ready=*/5)));
}

// The data completes exactly at the deadline. It used to be reported as a
// timeout, although it had been received (and consumed).
TEST(recv_timeout, data_completing_at_deadline_is_received) {
    constexpr int k_polls = (k_recv_timeout_ms / k_poll_interval_ms) - 1;
    EXPECT_TRUE(static_cast<bool>(recv_from_slow_client(
        /*now=*/5000, k_polls, /*read_ms=*/k_recv_timeout_ms - k_polls * k_poll_interval_ms)));
}

// A peer that never sends still times out after 30 seconds, also when
// millis() wraps around in the meantime
TEST(recv_timeout, silent_peer_times_out_after_timeout) {
    for (const tdsl::uint32_t now : {tdsl::uint32_t{5000}, tdsl::uint32_t{0xFFFFFFFFu - 1000u}}) {
        SCOPED_TRACE(now);
        const auto r = recv_from_slow_client(now, /*polls_until_ready=*/1000000);
        ASSERT_FALSE(static_cast<bool>(r));
        EXPECT_EQ(k_errc_timeout, r.error());
        EXPECT_EQ(static_cast<int>(k_recv_timeout_ms / k_poll_interval_ms),
                  mock_clock_scope::delays);
    }
}

// --------------------------------------------------------------------------------
// Send errors (#13) and segmentation (#14)
// --------------------------------------------------------------------------------

namespace {

    /**
     * scripted_client whose write() fails (writes nothing) from the
     * @ref fail_from_write th call on (1-based)
     */
    struct failing_write_client : public scripted_client {
        using scripted_client::scripted_client;

        tdsl::size_t write(const unsigned char * data, tdsl::size_t len) {
            if (++writes >= fail_from_write) {
                return 0;
            }
            return scripted_client::write(data, len);
        }

        void stop() {
            stopped = true;
        }

        int writes = {0};
        static int fail_from_write;
        static bool stopped;
    };

    int failing_write_client::fail_from_write = 1;
    bool failing_write_client::stopped        = false;

    struct failing_send_uut_t : public uut_t<failing_write_client> {
        using uut_t<failing_write_client>::uut_t;
        using uut_t<failing_write_client>::network_buffer;
    };

    using failing_cmd_ctx_t =
        tdsl::detail::command_context<tdsl::net::tdsl_netimpl_arduino<failing_write_client>>;
    using failing_tds_ctx_t = failing_cmd_ctx_t::tds_context_type;

    /**
     * End-of-message flags of the packets in @p tx
     */
    inline std::vector<tdsl::uint8_t> sent_packet_statuses(const byte_vec & tx) {
        std::vector<tdsl::uint8_t> r;
        tdsl::size_t off = 0;
        while (off + 8 <= tx.size()) {
            const auto len = static_cast<tdsl::uint16_t>((tx [off + 2] << 8) | tx [off + 3]);
            r.push_back(tx [off + 1]);
            if (len < 8) {
                break;
            }
            off += len;
        }
        return r;
    }
} // namespace

// A message of exactly two packets' worth of data used to be followed by
// an empty end-of-message packet
TEST(send, message_of_exact_packet_multiple_has_no_empty_packet) {
    tdsl::uint8_t nbuf [2048] = {0};
    byte_vec tx;
    recv_uut_t uut{nbuf, byte_vec{}, k_no_limit, k_no_limit, &tx};
    uut.set_tds_packet_size(512);

    EXPECT_EQ((packet_lengths{512, 512}), send_message(uut, tx, 2 * 504));
    EXPECT_EQ((std::vector<tdsl::uint8_t>{0x00, 0x01}), sent_packet_statuses(tx));

    EXPECT_EQ((packet_lengths{512}), send_message(uut, tx, 504));
    EXPECT_EQ((std::vector<tdsl::uint8_t>{0x01}), sent_packet_statuses(tx));

    EXPECT_EQ((packet_lengths{512, 512, 9}), send_message(uut, tx, 2 * 504 + 1));
    EXPECT_EQ((std::vector<tdsl::uint8_t>{0x00, 0x00, 0x01}), sent_packet_statuses(tx));
}

// An empty message is sent as a single, empty end-of-message packet
TEST(send, empty_message_is_a_single_packet) {
    tdsl::uint8_t nbuf [512] = {0};
    byte_vec tx;
    recv_uut_t uut{nbuf, byte_vec{}, k_no_limit, k_no_limit, &tx};
    EXPECT_EQ((packet_lengths{8}), send_message(uut, tx, 0));
    EXPECT_EQ((std::vector<tdsl::uint8_t>{0x01}), sent_packet_statuses(tx));
}

// The write of the second packet fails: the rest of the message is
// discarded, the connection is closed and the failure is reported
TEST(send, write_failure_is_reported) {
    tdsl::uint8_t nbuf [2048] = {0};
    byte_vec tx;
    failing_write_client::fail_from_write = 3; // 2nd packet's header
    failing_write_client::stopped         = false;
    failing_send_uut_t uut{nbuf, byte_vec{}, k_no_limit, k_no_limit, &tx};
    uut.set_tds_packet_size(512);

    const auto data = make_pattern(3 * 504);
    uut.do_write(tdsl::byte_view{data.data(), static_cast<tdsl::uint32_t>(data.size())});
    EXPECT_EQ(tdsl::detail::e_send_status::network_error,
              uut.do_send_tds_pdu(tdsl::detail::e_tds_message_type::sql_batch));
    EXPECT_EQ((packet_lengths{512}), sent_packet_lengths(tx)); // only the first one
    EXPECT_TRUE(failing_write_client::stopped);
    EXPECT_EQ(0u, uut.network_buffer.get_writer()->offset()); // nothing left behind
}

TEST(send, execute_query_reports_send_failure) {
    tdsl::uint8_t nbuf [512]              = {0};
    failing_write_client::fail_from_write = 1;
    failing_tds_ctx_t tds_ctx{nbuf, make_small_query_response()};
    failing_cmd_ctx_t cmd_ctx{tds_ctx};
    const auto r = cmd_ctx.execute_query(tdsl::string_view{"SELECT 0;"});
    EXPECT_TRUE(r.network_error);
    EXPECT_FALSE(static_cast<bool>(r));
}

TEST(send, execute_rpc_reports_send_failure) {
    tdsl::uint8_t nbuf [512]              = {0};
    failing_write_client::fail_from_write = 1;
    failing_tds_ctx_t tds_ctx{nbuf, make_small_query_response()};
    failing_cmd_ctx_t cmd_ctx{tds_ctx};
    tdsl::detail::sql_parameter_int p0{};
    tdsl::detail::sql_parameter_binding params [] = {p0};
    const auto r = cmd_ctx.execute_rpc(tdsl::string_view{"SELECT @p0"}, params);
    ASSERT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(tdsl::detail::e_rpc_error_code::network_error, r.error());
}

// --------------------------------------------------------------------------------
// Incomplete responses (connection lost while receiving)
// --------------------------------------------------------------------------------

// The connection drops in the middle of the response. This used to be
// reported as a successful query.
TEST(incomplete_response, execute_query_reports_network_error) {
    const auto stream = make_tds_packet(k_tabular_result, true, make_int_result_set(5));
    for (const tdsl::size_t cut : {tdsl::size_t{4}, tdsl::size_t{20}, stream.size() - 3}) {
        SCOPED_TRACE(cut);
        tdsl::uint8_t nbuf [512] = {0};
        e2e_tds_ctx_t tds_ctx{nbuf, stream, k_no_limit, /*fail_at=*/cut};
        e2e_cmd_ctx_t cmd_ctx{tds_ctx};
        const auto r = cmd_ctx.execute_query(tdsl::string_view{"SELECT n FROM t;"});
        EXPECT_TRUE(r.network_error);
        EXPECT_FALSE(static_cast<bool>(r));
    }
}

TEST(incomplete_response, execute_rpc_reports_network_error) {
    tdsl::uint8_t nbuf [512] = {0};
    const auto stream        = make_tds_packet(k_tabular_result, true, make_int_result_set(5));
    e2e_tds_ctx_t tds_ctx{nbuf, stream, k_no_limit, /*fail_at=*/20};
    e2e_cmd_ctx_t cmd_ctx{tds_ctx};
    tdsl::detail::sql_parameter_int p0{};
    tdsl::detail::sql_parameter_binding params [] = {p0};
    const auto r = cmd_ctx.execute_rpc(tdsl::string_view{"SELECT @p0"}, params);
    ASSERT_FALSE(static_cast<bool>(r));
    EXPECT_EQ(tdsl::detail::e_rpc_error_code::network_error, r.error());
}

// --------------------------------------------------------------------------------
// TEXT/NTEXT/IMAGE values (#15)
// --------------------------------------------------------------------------------

namespace {

    struct ntext_row_collector {
        static void cb(void * uptr, const tdsl::tds_colmetadata_token &,
                       const tdsl::tdsl_row & row) {
            auto & self = *static_cast<ntext_row_collector *>(uptr);
            self.text_null.push_back(row [0].is_null());
            const auto t = row [0].as<tdsl::u16char_view>();
            self.text.push_back(std::u16string(t.data(), t.size()));
            self.ints.push_back(row [1].as<tdsl::int32_t>());
        }

        std::vector<bool> text_null;
        std::vector<std::u16string> text;
        std::vector<tdsl::int32_t> ints;
    };

    inline void push_u32(byte_vec & v, tdsl::uint32_t x) {
        for (int s = 0; s < 32; s += 8) {
            v.push_back(static_cast<tdsl::uint8_t>(x >> s));
        }
    }

    /**
     * Result set with an NTEXT and an INT column, and two rows:
     * {NULL, 7} and {u"hi", 8}
     */
    inline byte_vec make_ntext_result_set() {
        byte_vec v{0x81,                         // COLMETADATA
                   0x02, 0x00,                   // column count
                   0x00, 0x00, 0x00, 0x00,       // user type, flags
                   0x63,                         // NTEXTTYPE
                   0xFE, 0xFF, 0xFF, 0x7F,       // max length
                   0x09, 0x04, 0xD0, 0x00, 0x34, // collation
                   0x01, 0x00, 0x74, 0x00,       // table name "t"
                   0x01, 0x63, 0x00,             // column name "c"
                   0x00, 0x00, 0x00, 0x00,       // user type, flags
                   0x38,                         // INT4TYPE
                   0x01, 0x6E, 0x00};            // column name "n"
        // Row 1: NULL NTEXT (zero-length text pointer), 7
        v.push_back(0xD1);
        v.push_back(0x00);
        push_u32(v, 7);
        // Row 2: u"hi", 8
        v.push_back(0xD1);
        v.push_back(0x10); // text pointer length
        for (int i = 0; i < 16; i++) {
            v.push_back(static_cast<tdsl::uint8_t>(0xA0 + i)); // text pointer
        }
        for (int i = 0; i < 8; i++) {
            v.push_back(static_cast<tdsl::uint8_t>(0xB0 + i)); // timestamp
        }
        push_u32(v, 4);
        const auto hi = ucs2("hi");
        v.insert(v.end(), hi.begin(), hi.end());
        push_u32(v, 8);
        const auto done = make_done_token(2);
        v.insert(v.end(), done.begin(), done.end());
        return v;
    }
} // namespace

// A NULL NTEXT value used to be read with an 8-byte timestamp that isn't
// there, consuming the next column's data
TEST(text_columns, null_and_non_null_ntext_values) {
    ntext_row_collector c;
    const auto r = run_query(make_ntext_result_set(), c);
    EXPECT_FALSE(r.response_error);
    EXPECT_TRUE(static_cast<bool>(r));
    EXPECT_EQ((std::vector<bool>{true, false}), c.text_null);
    EXPECT_EQ((std::vector<std::u16string>{u"", u"hi"}), c.text);
    EXPECT_EQ((std::vector<tdsl::int32_t>{7, 8}), c.ints);
}

// The same result set split into two packets at every position
TEST(text_columns, ntext_result_set_split_at_every_position) {
    const auto rs = make_ntext_result_set();
    for (tdsl::size_t split = 1; split < rs.size(); split++) {
        SCOPED_TRACE(split);
        ntext_row_collector c;
        const auto r = run_query_stream(split_into_two_packets(rs, split), c);
        EXPECT_TRUE(static_cast<bool>(r));
        EXPECT_EQ((std::vector<tdsl::int32_t>{7, 8}), c.ints);
    }
}

// --------------------------------------------------------------------------------
// Login and connection parameter limits
// --------------------------------------------------------------------------------

// LOGIN7 strings are limited to 128 characters
TEST(login_limits, login_string_longer_than_128_chars_is_rejected) {
    tdsl::uint8_t nbuf [4096] = {0};
    byte_vec tx;
    driver_t driver{nbuf, byte_vec{}, k_no_limit, k_no_limit, &tx};
    const std::string long_name(129, 'a');
    driver_t::connection_parameters params;
    params.server_name = "server";
    params.user_name   = "user";
    params.password    = "pass";
    params.app_name    = as_sv(long_name);
    EXPECT_EQ(driver_t::e_driver_error_code::login_failed, driver.connect(params));
    EXPECT_TRUE(tx.empty());
}

TEST(login_limits, login_string_of_128_chars_is_sent) {
    tdsl::uint8_t nbuf [4096] = {0};
    byte_vec tx;
    driver_t driver{nbuf, byte_vec{}, k_no_limit, k_no_limit, &tx};
    const std::string name(128, 'a');
    driver_t::connection_parameters params;
    params.server_name = "server";
    params.user_name   = "user";
    params.password    = "pass";
    params.app_name    = as_sv(name);
    // No response, so the login fails, but the request is sent
    EXPECT_EQ(driver_t::e_driver_error_code::login_failed, driver.connect(params));
    EXPECT_FALSE(tx.empty());
}

// The retry parameters used to be truncated to 16 bits silently
TEST(login_limits, retry_parameters_larger_than_16_bits_are_rejected) {
    tdsl::uint8_t nbuf [512] = {0};
    driver_t driver{nbuf, byte_vec{}};
    driver_t::connection_parameters params;
    params.server_name         = "server";
    params.conn_retry_delay_ms = 70000;
    EXPECT_EQ(driver_t::e_driver_error_code::connection_param_retry_invalid, params.validate());
    EXPECT_EQ(driver_t::e_driver_error_code::connection_param_retry_invalid,
              driver.connect(params));
    params.conn_retry_delay_ms = 65535;
    params.conn_retry_count    = 65536;
    EXPECT_EQ(driver_t::e_driver_error_code::connection_param_retry_invalid, params.validate());
    params.conn_retry_count = 65535;
    EXPECT_EQ(driver_t::e_driver_error_code::success, params.validate());
}

// --------------------------------------------------------------------------------
// tdsl_netimpl_arduino byte_span constructor (latent: could not be called)
// --------------------------------------------------------------------------------

TEST(netimpl_arduino, construct_with_byte_span) {
    tdsl::uint8_t storage [64] = {0};
    host_uut_t uut{
        tdsl::byte_span{storage, sizeof(storage)}
    };
    EXPECT_EQ(storage, uut.network_buffer.get_underlying_view().data());
    EXPECT_EQ(sizeof(storage), uut.network_buffer.get_underlying_view().size_bytes());
}
