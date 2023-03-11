/**
 * ____________________________________________________
 * binary_writer class unit tests
 *
 * @file   ut_binary_writer.cpp
 * @author mkg <me@mustafagilor.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <tdslite/util/tdsl_endian.hpp>
#include <tdslite/util/tdsl_binary_writer.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <gtest/gtest.h>

// --------------------------------------------------------------------------------

constexpr tdsl::uint8_t source_buffer [] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};

// --------------------------------------------------------------------------------

struct binary_writer_test : public ::testing::Test {
    template <tdsl::endian E = tdsl::endian::native>
    using uut                             = tdsl::binary_writer<E>;
    tdsl::uint8_t destination_buffer [16] = {};
};

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, construct) {
    EXPECT_NO_THROW({
        uut<> aa{destination_buffer};
        (void) aa;
    });
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, construct_explicit_size) {

    EXPECT_NO_THROW(([this] {
        uut<> aa{&destination_buffer [0], sizeof(destination_buffer)};
        (void) aa;
    }()));
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, construct_span) {

    tdsl::byte_span data{&destination_buffer [4], &destination_buffer [8]};

    EXPECT_NO_THROW(([&data] {
        uut<> writer{data};
        (void) writer;
    }()));
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, write_span) {

    uut<> writer{destination_buffer};
    constexpr tdsl::byte_view data_to_write{&source_buffer [0], &source_buffer [4]};

    auto result = writer.write(data_to_write);
    ASSERT_TRUE(result);
    ASSERT_EQ(data_to_write.size_bytes(), writer.offset());
    ASSERT_EQ(sizeof(destination_buffer) - data_to_write.size_bytes(), writer.remaining_bytes());
    ASSERT_EQ(std::distance(writer.inuse_begin(), writer.inuse_end()), data_to_write.size_bytes());

    for (auto itr = writer.inuse_begin(), ite = data_to_write.begin(); itr != writer.inuse_end();
         itr++, ite++) {
        EXPECT_EQ(*itr, *ite);
    }
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, seek) {

    uut<> writer{destination_buffer};
    constexpr tdsl::byte_view data_to_write{&source_buffer [0], &source_buffer [4]};
    ASSERT_TRUE(writer.seek(4));
    constexpr int expected_offset = data_to_write.size_bytes() + 4;
    ASSERT_TRUE(writer.write(data_to_write));
    ASSERT_EQ(expected_offset, writer.offset());
    ASSERT_EQ(sizeof(destination_buffer) - expected_offset, writer.remaining_bytes());
    ASSERT_EQ(std::distance(writer.inuse_begin(), writer.inuse_end()), expected_offset);

    tdsl::uint8_t expected_bytes [] = {0x00,
                                       0x00,
                                       0x00,
                                       0x00,
                                       data_to_write [0],
                                       data_to_write [1],
                                       data_to_write [2],
                                       data_to_write [3]};
    tdsl::byte_view expected_bytes_vw{expected_bytes};
    ASSERT_EQ(expected_bytes_vw.size_bytes(), writer.offset());
    for (auto itr = writer.inuse_begin(), ite = expected_bytes_vw.begin();
         itr != writer.inuse_end(); itr++, ite++) {
        EXPECT_EQ(*itr, *ite);
    }
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, positive_advance) {

    uut<> writer{destination_buffer};
    ASSERT_TRUE(writer.advance(2));
    constexpr tdsl::byte_view data_to_write{&source_buffer [0], &source_buffer [4]};
    ASSERT_TRUE(writer.write(data_to_write));
    constexpr int expected_offset = data_to_write.size_bytes() + 2;
    ASSERT_EQ(expected_offset, writer.offset());
    ASSERT_EQ(sizeof(destination_buffer) - expected_offset, writer.remaining_bytes());
    ASSERT_EQ(std::distance(writer.inuse_begin(), writer.inuse_end()), expected_offset);

    tdsl::uint8_t expected_bytes [] = {
        0x00, 0x00, data_to_write [0], data_to_write [1], data_to_write [2], data_to_write [3]};

    tdsl::byte_view expected_bytes_vw{expected_bytes};

    ASSERT_EQ(expected_bytes_vw.size_bytes(), writer.offset());
    for (auto itr = writer.inuse_begin(), ite = expected_bytes_vw.begin();
         itr != writer.inuse_end(); itr++, ite++) {
        EXPECT_EQ(*itr, *ite);
    }

    // OOB advance
    ASSERT_TRUE(writer.advance(sizeof(destination_buffer) - expected_offset));
    ASSERT_FALSE(writer.advance(1));
    ASSERT_TRUE(writer.advance(0));
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, negative_advance) {
    uut<> writer{destination_buffer};
    ASSERT_TRUE(writer.advance(sizeof(destination_buffer)));
    ASSERT_EQ(writer.remaining_bytes(), 0);
    ASSERT_TRUE(writer.advance(static_cast<tdsl::int32_t>(-sizeof(destination_buffer))));
    ASSERT_FALSE(writer.advance(-1));
    ASSERT_EQ(writer.remaining_bytes(), sizeof(destination_buffer));
    constexpr tdsl::byte_view data_to_write{&source_buffer [0], &source_buffer [4]};
    ASSERT_TRUE(writer.write(data_to_write));
    ASSERT_EQ(std::distance(writer.inuse_begin(), writer.inuse_end()),
              std::distance(data_to_write.begin(), data_to_write.end()));

    for (auto itr = writer.inuse_begin(), ite = data_to_write.begin(); itr != writer.inuse_end();
         itr++, ite++) {
        EXPECT_EQ(*itr, *ite);
    }
    ASSERT_TRUE(writer.advance(static_cast<tdsl::int32_t>(-data_to_write.size_bytes())));
    constexpr tdsl::byte_view data_to_write2{&source_buffer [4], &source_buffer [8]};
    ASSERT_TRUE(writer.write(data_to_write2));
    ASSERT_EQ(std::distance(writer.inuse_begin(), writer.inuse_end()),
              std::distance(data_to_write2.begin(), data_to_write2.end()));

    for (auto itr = writer.inuse_begin(), ite = data_to_write2.begin(); itr != writer.inuse_end();
         itr++, ite++) {
        EXPECT_EQ(*itr, *ite);
    }
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, remaining_bytes) {
    uut<> writer{destination_buffer};
    ASSERT_EQ(writer.remaining_bytes(), sizeof(destination_buffer));
    writer.seek(2);
    ASSERT_EQ(writer.remaining_bytes(), sizeof(destination_buffer) - 2);
    writer.advance(2);
    ASSERT_EQ(writer.remaining_bytes(), sizeof(destination_buffer) - 4);
    writer.advance(12);
    ASSERT_EQ(writer.remaining_bytes(), 0);
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, has_bytes) {
    uut<> writer{destination_buffer};
    ASSERT_TRUE(writer.has_bytes(sizeof(destination_buffer)));
    ASSERT_FALSE(writer.has_bytes(tdsl::uint32_t{0xFFFFFFFF}));
    ASSERT_TRUE(writer.has_bytes(tdsl::uint32_t{0x0}));
    ASSERT_TRUE(writer.seek(2));
    ASSERT_TRUE(writer.has_bytes(sizeof(destination_buffer) - 2));
    ASSERT_FALSE(writer.has_bytes(sizeof(destination_buffer) - 1));
    ASSERT_TRUE(writer.advance(2));
    ASSERT_TRUE(writer.has_bytes(sizeof(destination_buffer) - 4));
    ASSERT_FALSE(writer.has_bytes(sizeof(destination_buffer) - 3));
    ASSERT_TRUE(writer.advance(12));
    ASSERT_TRUE(writer.has_bytes(0));
    ASSERT_FALSE(writer.has_bytes(1));
    // overread
    ASSERT_TRUE(writer.advance(-1));
    ASSERT_FALSE(writer.advance(2));
    ASSERT_TRUE(writer.advance(1));
    ASSERT_FALSE(writer.advance(1));
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, reset) {
    uut<> writer{destination_buffer};
    ASSERT_TRUE(writer.seek(4));
    writer.reset();
    ASSERT_EQ(writer.offset(), 0);
    ASSERT_EQ(writer.remaining_bytes(), sizeof(destination_buffer));
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, endianness_ne2be) {
    uut<tdsl::endian::big> writer{destination_buffer};
    ASSERT_TRUE(writer.write(tdsl::uint32_t{0x01020304}));
    tdsl::uint8_t expected []{0x01, 0x02, 0x03, 0x04};
    tdsl::byte_view expected_vw{expected};
    for (auto itr = writer.inuse_begin(), ite = expected_vw.begin(); itr != writer.inuse_end();
         itr++, ite++) {
        EXPECT_EQ(*itr, *ite);
    }
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, endianness_ne2le) {
    uut<tdsl::endian::little> writer{destination_buffer};
    ASSERT_TRUE(writer.write(tdsl::uint32_t{0x01020304}));
    tdsl::uint8_t expected []{0x04, 0x03, 0x02, 0x01};
    tdsl::byte_view expected_vw{expected};
    for (auto itr = writer.inuse_begin(), ite = expected_vw.begin(); itr != writer.inuse_end();
         itr++, ite++) {
        EXPECT_EQ(*itr, *ite);
    }
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, endianness_ne2ne) {
    uut<tdsl::endian::native> writer{destination_buffer};
    tdsl::uint32_t v{0x01020304};
    ASSERT_TRUE(writer.write(v));
    const auto & expected = reinterpret_cast<const tdsl::uint8_t(&) [4]>(v);
    tdsl::byte_view expected_vw{expected};
    for (auto itr = writer.inuse_begin(), ite = expected_vw.begin(); itr != writer.inuse_end();
         itr++, ite++) {
        EXPECT_EQ(*itr, *ite);
    }
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, endianness_override) {
    uut<tdsl::endian::native> writer{destination_buffer};
    tdsl::uint32_t nv{0x01020304};
    tdsl::uint32_t nnv = tdsl::swap_endianness(nv);
    ASSERT_TRUE(writer.write<tdsl::endian::non_native>(nv));
    const auto & expected = reinterpret_cast<const tdsl::uint8_t(&) [4]>(nnv);
    tdsl::byte_view expected_vw{expected};
    for (auto itr = writer.inuse_begin(), ite = expected_vw.begin(); itr != writer.inuse_end();
         itr++, ite++) {
        EXPECT_EQ(*itr, *ite);
    }
}

// --------------------------------------------------------------------------------

TEST_F(binary_writer_test, checkpoint) {
    uut<tdsl::endian::native> writer{destination_buffer};
    ASSERT_TRUE(writer.write(tdsl::uint32_t{0x01020304}));
    auto cp = writer.checkpoint();
    ASSERT_TRUE(writer.write(tdsl::uint32_t{0x01020304}));
    cp.restore();
    ASSERT_EQ(sizeof(tdsl::uint32_t), writer.offset());
}
