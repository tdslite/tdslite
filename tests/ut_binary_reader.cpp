/**
 * ____________________________________________________
 * tdslite main unit test
 *
 * @file   ut_tdslite.cpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   12.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include "tdslite/detail/tds_endian.hpp"
#include <tdslite/detail/tds_binary_reader.hpp>
#include <gtest/gtest.h>

class binary_reader_f : public ::testing::Test {};

TEST_F(binary_reader_f, construct) {
    uint8_t buf [80];
    ASSERT_NO_THROW(tdslite::binary_reader<tdslite::endian::little>{buf};);
    ASSERT_NO_THROW(tdslite::binary_reader<tdslite::endian::big>{buf});
}