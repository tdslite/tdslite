/**
 * ____________________________________________________
 * binary_reader class unit tests
 *
 * @file   ut_binary_reader.cpp
 * @author mkg <me@mustafagilor.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <tdslite/util/tdsl_endian.hpp>
#include <tdslite/util/tdsl_binary_reader.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>
#include <gtest/gtest.h>

// --------------------------------------------------------------------------------

constexpr tdsl::uint8_t buffer_to_read [] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                                             0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10};

// --------------------------------------------------------------------------------

TEST(binary_reader_test, construct) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    EXPECT_NO_THROW({
        uut aa{buffer_to_read};
        (void) aa;
    });
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, construct_explicit_size) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;

    EXPECT_NO_THROW(([] {
        uut aa{&buffer_to_read [0], sizeof(buffer_to_read)};
        (void) aa;
    }()));
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, construct_span) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    constexpr uut::span_type data{&buffer_to_read [4], &buffer_to_read [8]};

    EXPECT_NO_THROW(([&data] {
        uut reader{data};
        (void) reader;
    }()));
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, read_span) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    uut reader{buffer_to_read};
    constexpr uut::span_type expected_result{&buffer_to_read [0], &buffer_to_read [4]};
    auto result = reader.read(4);
    ASSERT_TRUE(result);
    ASSERT_EQ(expected_result.size_bytes(), result.size_bytes());

    for (auto itr = result.begin(), ite = expected_result.begin(); itr != result.end();
         itr++, ite++) {
        EXPECT_EQ(*itr, *ite);
    }
}

// --------------------------------------------------------------------------------

// TEST(binary_reader_test, reinterpret_read) {
//     using uut = tdsl::binary_reader<tdsl::endian::native>;

//     struct NCF_PACKED target_type {
//         tdsl::uint8_t u1;
//         tdsl::uint8_t u2;
//         std::uint16_t u3;
//         std::uint32_t u4;
//         auto operator<=>(const target_type &) const = default;
//     };

//     constexpr auto expected_result = target_type{
//         .u1 = 0x01,
//         .u2 = 0x02,
//         .u3 = (tdsl::endian::native == tdsl::endian::little ? std::uint16_t(0x0403) :
//         std::uint16_t(0x0304)), .u4 = (tdsl::endian::native == tdsl::endian::little ?
//         std::uint32_t(0x08070605) : std::uint32_t(0x05060708)),
//     };

//     uut reader{buffer_to_read};
//     auto result = reader.reinterpret_read<const target_type *>();
//     EXPECT_EQ(*result, expected_result);
// }

// --------------------------------------------------------------------------------

TEST(binary_reader_test, seek) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    uut reader{buffer_to_read};
    ASSERT_TRUE(reader.seek(4));
    auto result = reader.read(4);
    ASSERT_TRUE(result);

    constexpr uut::span_type expected_result{&buffer_to_read [4], &buffer_to_read [8]};

    ASSERT_EQ(expected_result.size_bytes(), result.size_bytes());

    for (auto itr = result.begin(), ite = expected_result.begin(); itr != result.end();
         itr++, ite++) {
        EXPECT_EQ(*itr, *ite);
    }
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, advance) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    uut reader{buffer_to_read};
    ASSERT_TRUE(reader.advance(2));
    auto result = reader.read(6);

    constexpr uut::span_type expected_result{&buffer_to_read [2], &buffer_to_read [8]};

    ASSERT_EQ(expected_result.size_bytes(), result.size_bytes());

    for (auto itr = result.begin(), ite = expected_result.begin(); itr != result.end();
         itr++, ite++) {
        EXPECT_EQ(*itr, *ite);
    }
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, remaining_bytes) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    uut reader{buffer_to_read};
    auto result = reader.read(6);
    EXPECT_TRUE(result);
    EXPECT_EQ(sizeof(buffer_to_read) - 6, reader.remaining_bytes());
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, has_bytes) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    uut reader{buffer_to_read};
    auto result = reader.read(6);
    EXPECT_TRUE(result);
    EXPECT_TRUE(reader.has_bytes(2));
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, has_bytes_false) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    uut reader{buffer_to_read};
    auto result = reader.read(14);
    EXPECT_TRUE(result);
    EXPECT_FALSE(reader.has_bytes(3));
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, reset) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    uut reader{buffer_to_read};
    ASSERT_TRUE(reader.seek(4));
    reader.reset();
    auto result = reader.read(4);
    EXPECT_TRUE(result);
    constexpr uut::span_type expected_result{&buffer_to_read [0], &buffer_to_read [4]};

    ASSERT_EQ(expected_result.size_bytes(), result.size_bytes());

    for (auto itr = result.begin(), ite = expected_result.begin(); itr != result.end();
         itr++, ite++) {
        EXPECT_EQ(*itr, *ite);
    }
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, read_all) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    uut reader{buffer_to_read};
    EXPECT_TRUE(reader.advance(sizeof(buffer_to_read)));
    EXPECT_FALSE(reader.has_bytes(1));
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, overread) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    uut reader{buffer_to_read};
    EXPECT_FALSE(reader.advance(sizeof(buffer_to_read) + 1));
    EXPECT_TRUE(reader.has_bytes(1));
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, overread_2) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    uut reader{buffer_to_read};
    EXPECT_FALSE(reader.advance(0xffffffff));
    EXPECT_TRUE(reader.has_bytes(1));
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, subreader) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    uut reader{buffer_to_read};
    auto a = reader.read<tdsl::uint8_t>();
    (void) a;
    auto sr = reader.subreader(sizeof(buffer_to_read) - sizeof(tdsl::uint8_t));
    EXPECT_TRUE(sr);
    EXPECT_EQ(sr.current(), reader.current());
    EXPECT_EQ(sr.size_bytes(), sizeof(buffer_to_read) - sizeof(tdsl::uint8_t));
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, subreader_clamp) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    uut reader{buffer_to_read};
    auto a = reader.read<tdsl::uint8_t>();
    (void) a;
    auto sr = reader.subreader(sizeof(buffer_to_read)); // exceed the size
    EXPECT_FALSE(sr);
}

// --------------------------------------------------------------------------------

TEST(binary_reader_test, checkpoint) {
    using uut = tdsl::binary_reader<tdsl::endian::native>;
    uut reader{buffer_to_read};
    auto a = reader.read<tdsl::uint8_t>();
    (void) a;
    auto cp = reader.checkpoint();
    auto b  = reader.read<tdsl::uint8_t>();
    (void) b;
    cp.restore();
    ASSERT_EQ(reader.offset(), 1);
}