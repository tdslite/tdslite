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
                        tdsl::size_t fail_at = k_no_limit) :
            stream(stream), max_read(max_read), fail_at(fail_at) {}

        tdsl::size_t write(const unsigned char *, tdsl::size_t len) {
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