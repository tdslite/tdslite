/**
 * ____________________________________________________
 * tdsl_buffer_object class unit tests
 *
 * @file   ut_buffer_object.cpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   18.11.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <tdslite/util/tdsl_buffer_object.hpp>
#include <tdslite/util/tdsl_hex_dump.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

struct buffer_object_fixture : public ::testing::Test {
    tdsl::uint8_t buf [8192] = {};
    tdsl::tdsl_buffer_object bo{buf};
};

// --------------------------------------------------------------------------------

/**
 * Test sequential write-reads.
 */
TEST_F(buffer_object_fixture, write_read_buf_partial) {
    tdsl::uint8_t wbuf [32] = {};
    // Fill first half with 0xff's
    std::fill(wbuf, &wbuf [16], 0xff);
    // Fill second half with 0xcc's
    std::fill(&wbuf [16], &wbuf [32], 0xcc);
    for (int i = 0; i < 10; i++) {
        {
            auto writer = bo.get_writer();
            ASSERT_TRUE(writer->write(tdsl::byte_view{wbuf}));
            ASSERT_EQ(writer->offset(), 32);
            ASSERT_EQ(writer->size_bytes(), sizeof(buf));
            ASSERT_EQ(writer->remaining_bytes(), sizeof(buf) - sizeof(wbuf));
        }
        // Enable for diagnostics:
        // {
        //     auto ulv = bo.get_underlying_view();
        //     tdsl::util::hexdump(ulv.begin(), 64);
        // }
        {
            auto reader = bo.get_reader();
            auto rw     = reader->read(/*number_of_elements=*/16);
            ASSERT_THAT(rw, testing::Contains(0xff).Times(16));
            ASSERT_EQ(reader->remaining_bytes(), 16);
        }
        // Enable for diagnostics:
        // {
        //     printf("------------------------------------------------\n");
        //     auto ulv = bo.get_underlying_view();
        //     tdsl::util::hexdump(ulv.begin(), 64);
        // }
        {
            auto reader = bo.get_reader();
            auto rw     = reader->read(/*number_of_elements=*/16);
            ASSERT_THAT(rw, testing::Contains(0xcc).Times(16));
            ASSERT_EQ(reader->remaining_bytes(), 0);
        }
    }
}

// --------------------------------------------------------------------------------

/**
 * Test sequential write-reads.
 */
TEST_F(buffer_object_fixture, write_overflow) {
    tdsl::uint8_t wbuf [32] = {};
    // Fill first half with 0xff's
    std::fill(wbuf, &wbuf [16], 0xff);
    // Fill second half with 0xcc's
    std::fill(&wbuf [16], &wbuf [32], 0xcc);

    for (tdsl::uint32_t i = 0; i < sizeof(buf) / sizeof(wbuf); i++) {
        ASSERT_TRUE(bo.get_writer()->write(wbuf));
    }

    ASSERT_FALSE(bo.get_writer()->write(tdsl::uint8_t{1}));
    ASSERT_EQ(bo.get_reader()->read<tdsl::uint8_t>(), 0xff);
    ASSERT_TRUE(bo.get_writer()->write(tdsl::uint8_t{1}));
    ASSERT_EQ(bo.get_reader()->read<tdsl::uint8_t>(), 0xff);
}

// --------------------------------------------------------------------------------

/**
 * Test sequential write-reads.
 */
TEST_F(buffer_object_fixture, read_overflow) {
    tdsl::uint8_t v{};
    ASSERT_DEATH(v = bo.get_reader()->read<tdsl::uint8_t>(), "");
    (void) v;
}

// --------------------------------------------------------------------------------

/**
 * Test sequential write-reads.
 */
TEST_F(buffer_object_fixture, write_1) {
    for (tdsl::uint32_t i = 0; i < sizeof(buf); i++) {
        ASSERT_TRUE(bo.get_writer()->write(static_cast<tdsl::uint8_t>(i % 256)));
    }
    for (tdsl::uint32_t i = 0; i < sizeof(buf); i++) {
        ASSERT_NO_FATAL_FAILURE({
            tdsl::uint8_t v{};
            v = bo.get_reader()->read<tdsl::uint8_t>();
            (void) v;
        });
    }
    ASSERT_EQ(bo.get_reader()->remaining_bytes(), 0);
    ASSERT_EQ(bo.get_writer()->remaining_bytes(), sizeof(buf));
}

// --------------------------------------------------------------------------------

/**
 * Test sequential write-reads.
 */
TEST_F(buffer_object_fixture, write_4) {
    for (tdsl::uint32_t i = 0; i < (sizeof(buf) / 4); i++) {
        ASSERT_TRUE(bo.get_writer()->write(tdsl::uint32_t{i % 256}));
        ASSERT_EQ(bo.get_reader()->remaining_bytes(), ((i + 1) * sizeof(tdsl::uint32_t)));
        ASSERT_EQ(bo.get_writer()->remaining_bytes(),
                  sizeof(buf) - ((i + 1) * sizeof(tdsl::uint32_t)));
    }
    for (tdsl::uint32_t i = 0; i < (sizeof(buf) / 4); i++) {
        ASSERT_NO_FATAL_FAILURE({
            tdsl::uint8_t v{};
            v = bo.get_reader()->read<tdsl::uint32_t>();
            (void) v;
        });
        ASSERT_EQ(bo.get_reader()->remaining_bytes(),
                  sizeof(buf) - ((i + 1) * sizeof(tdsl::uint32_t)));
        ASSERT_EQ(bo.get_writer()->remaining_bytes(), ((i + 1) * sizeof(tdsl::uint32_t)));
    }
    ASSERT_EQ(bo.get_reader()->remaining_bytes(), 0);
    ASSERT_EQ(bo.get_writer()->remaining_bytes(), sizeof(buf));
}

// --------------------------------------------------------------------------------

/**
 * Test sequential write-reads.
 */
TEST_F(buffer_object_fixture, write_read_int_seq) {
    for (int i = 0; i < 10; i++) {
        {
            auto writer = bo.get_writer();
            ASSERT_TRUE(writer->write(/*value=*/tdsl::uint32_t{5}));
            ASSERT_EQ(writer->offset(), 4);
            ASSERT_EQ(writer->size_bytes(), sizeof(buf));
            ASSERT_EQ(writer->remaining_bytes(), sizeof(buf) - 4);
        }
        {
            auto reader = bo.get_reader();
            ASSERT_EQ(reader->read<tdsl::uint32_t>(), 5);
            ASSERT_EQ(reader->remaining_bytes(), 0);
        }
    }
}

// --------------------------------------------------------------------------------

/**
 * Test sequential write-reads.
 */
TEST_F(buffer_object_fixture, write_read_buf) {
    tdsl::uint8_t wbuf [32] = {0xFF};
    for (int i = 0; i < 10; i++) {
        {
            auto writer = bo.get_writer();
            ASSERT_TRUE(writer->write(tdsl::byte_view{wbuf}));
            ASSERT_EQ(writer->offset(), 32);
            ASSERT_EQ(writer->size_bytes(), sizeof(buf));
            ASSERT_EQ(writer->remaining_bytes(), sizeof(buf) - sizeof(wbuf));
        }
        {
            auto reader = bo.get_reader();
            auto rw     = reader->read(sizeof(wbuf));
            ASSERT_THAT(rw, testing::ElementsAreArray(wbuf));
            ASSERT_EQ(reader->remaining_bytes(), 0);
        }
    }
}

// --------------------------------------------------------------------------------

/**
 * Test reader-writer exclusivity.
 *
 * The reader/writer API is not meant to be used concurrently
 * and one buffer object can only be modified by exactly one reader
 * or writer at any given time. If two readers/writers are active
 * at the same time, it is a programming error and doing so will
 * raise an assertion in debug mode, and cause an std::abort
 */
TEST_F(buffer_object_fixture, exclusivity) {

#ifdef NDEBUG
#define exclusivity_EXPECT_DEATH(A, B) EXPECT_DEATH(A, B)
#else
#define exclusivity_EXPECT_DEATH(A, B) EXPECT_DEBUG_DEATH(A, B)
#endif
    // writer-reader
    {
        auto v = bo.get_writer();
        exclusivity_EXPECT_DEATH(
            {
                auto x = bo.get_reader();
                (void) x;
            },
            "");
    }

    // writer-reader
    {
        auto v = bo.get_writer();
        exclusivity_EXPECT_DEATH(
            {
                auto x = bo.get_writer();
                (void) x;
            },
            "");
    }

    // reader-writer
    {
        auto v = bo.get_reader();
        exclusivity_EXPECT_DEATH(
            {
                auto x = bo.get_writer();
                (void) x;
            },
            "");
    }

    // reader-reader
    {
        auto v = bo.get_reader();
        exclusivity_EXPECT_DEATH(
            {
                auto x = bo.get_reader();
                (void) x;
            },
            "");
    }

    // Exclusive writers
    for (int i = 0; i < 10; i++) {
        ASSERT_NO_FATAL_FAILURE({
            auto x = bo.get_writer();
            (void) x;
        });
    }

    // Exclusive readers
    for (int i = 0; i < 10; i++) {
        ASSERT_NO_FATAL_FAILURE({
            auto x = bo.get_reader();
            (void) x;
        });
    }
}

//////////////////