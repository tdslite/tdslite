/**
 * ____________________________________________________
 * tdsl_field class unit tests
 *
 * @file   ut_tdsl_field.cpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   05.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <tdslite/detail/tdsl_field.hpp>
#include <tdslite/util/tdsl_span.hpp>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstring>

using uut_t = tdsl::tdsl_field;

struct tdsl_field_fixture : public ::testing::Test {
    uut_t field{};
};

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_uint8) {
    tdsl::uint8_t buf [1] = {0x01};
    field                 = buf;
    EXPECT_EQ(field.as<tdsl::uint8_t>(), tdsl::uint8_t{1});
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_uint16) {
    tdsl::uint8_t buf [2] = {0x01, 0x02};
    field                 = buf;
    EXPECT_EQ(field.as<tdsl::uint16_t>(), tdsl::uint16_t{513});
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_uint32) {
    tdsl::uint8_t buf [4] = {0x01, 0x02, 0x03, 0x04};
    field                 = buf;
    EXPECT_EQ(field.as<tdsl::uint32_t>(), tdsl::uint32_t{67305985});
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_uint64) {
    tdsl::uint8_t buf [8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    field                 = buf;
    EXPECT_EQ(field.as<tdsl::uint64_t>(), tdsl::uint64_t{578437695752307201});
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_sqlmoney) {
    tdsl::uint8_t buf1 [8] = {0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00};
    tdsl::uint8_t buf2 [8] = {0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff};
    field                  = buf1;
    EXPECT_EQ(field.as<tdsl::sqltypes::sql_money>().raw(),
              tdsl::numeric_limits::min_value<tdsl::int64_t>());
    EXPECT_EQ(field.as<tdsl::sqltypes::sql_money>(), -922337203685477.5808);

    field = buf2;
    EXPECT_EQ(field.as<tdsl::sqltypes::sql_money>().raw(),
              tdsl::numeric_limits::max_value<tdsl::int64_t>());
    EXPECT_EQ(field.as<tdsl::sqltypes::sql_money>(), 922337203685477.5807);
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_int64) {
    tdsl::uint8_t buf [8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    field                 = buf;
    EXPECT_EQ(field.as<tdsl::uint64_t>(), tdsl::uint64_t{578437695752307201});
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_string_view) {
    constexpr tdsl::uint8_t buf [14] = {0x74, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73,
                                        0x20, 0x61, 0x20, 0x74, 0x65, 0x73, 0x74};
    field                            = buf;
    const constexpr char str []      = "this is a test";
    tdsl::char_view expected_span{str, sizeof(str) - 1};
    ASSERT_THAT(field.as<tdsl::char_view>(), testing::ElementsAreArray(expected_span));
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_u16char_view) {
    constexpr tdsl::uint8_t buf [28] = {0x74, 0x00, 0x68, 0x00, 0x69, 0x00, 0x73, 0x00, 0x20, 0x00,
                                        0x69, 0x00, 0x73, 0x00, 0x20, 0x00, 0x61, 0x00, 0x20, 0x00,
                                        0x74, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00};
    field                            = buf;
    const constexpr char16_t str []  = u"this is a test";
    tdsl::u16char_view expected_span{str, (sizeof(str) / sizeof(char16_t)) - 1};
    auto result = field.as<tdsl::u16char_view>();
    ASSERT_EQ(result.size(), 14);
    ASSERT_EQ(result.size_bytes(), 28);
    ASSERT_THAT(result, testing::ElementsAreArray(expected_span));
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_fixture, field_as_u32string_view) {
    constexpr tdsl::uint8_t buf [56] = {
        0x74, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x73, 0x00,
        0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00,
        0x20, 0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x74, 0x00,
        0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00};
    field                           = buf;
    const constexpr char32_t str [] = U"this is a test";
    tdsl::u32char_view expected_span{str, (sizeof(str) / sizeof(char32_t)) - 1};
    auto result = field.as<tdsl::u32char_view>();
    ASSERT_EQ(result.size(), 14);
    ASSERT_EQ(result.size_bytes(), 56);
    ASSERT_THAT(result, testing::ElementsAreArray(expected_span));
}

// --------------------------------------------------------------------------------

#include <tdslite/detail/token/tds_colmetadata_token.hpp>
#include <tdslite/detail/tdsl_data_type.hpp>

bool field_to_string(const tdsl::tds_column_info & colinfo, const tdsl::tdsl_field & field,
                     tdsl::char_span buf,
                     int (*snprintf_fn)(char *, size_t n, const char *, ...) = std::snprintf) {

    using data_type = tdsl::detail::e_tds_data_type;
    switch (colinfo.type) {
        case data_type::NULLTYPE:
            snprintf_fn(buf.data(), buf.size_bytes(), "<NULL>");
            break;
        case data_type::INT1TYPE:
            snprintf_fn(buf.data(), buf.size_bytes(), "%d", field.as<tdsl::int8_t>());
            break;
        case data_type::INTNTYPE:

            switch (colinfo.typeprops.u8l.length) {
                case 1:
                    snprintf_fn(buf.data(), buf.size_bytes(), "%d", field.as<tdsl::int8_t>());
                    break;
                case 2:
                    snprintf_fn(buf.data(), buf.size_bytes(), "%d", field.as<tdsl::int16_t>());
                    break;
                case 4:
                    snprintf_fn(buf.data(), buf.size_bytes(), "%ld", field.as<tdsl::int32_t>());
                    break;
                case 8:
                    snprintf_fn(buf.data(), buf.size_bytes(), "%lld", field.as<tdsl::int64_t>());
                    break;
                default:
                    snprintf_fn(buf.data(), buf.size_bytes(), "<INVALID>");
                    return false;
            }

            break;
        case data_type::BITTYPE:
            snprintf_fn(buf.data(), buf.size_bytes(),
                        (field.as<tdsl::int8_t>() == 0 ? "FALSE" : "TRUE"));
            break;
        case data_type::GUIDTYPE: {
            tdsl::binary_reader<tdsl::endian::little> guid_reader{field};
            constexpr int k_guid_size = 16;
            if (guid_reader.size_bytes() == k_guid_size) {
                const auto time_low          = guid_reader.read<tdsl::uint32_t>();
                const auto time_mid          = guid_reader.read<tdsl::uint16_t>();
                const auto time_hi_ver       = guid_reader.read<tdsl::uint16_t>();
                const auto cseqh_rcseql_node = guid_reader.read(/*number_of_elements=*/8);
                snprintf_fn(buf.data(), buf.size_bytes(),
                            "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x", time_low, time_mid,
                            time_hi_ver, cseqh_rcseql_node [0], cseqh_rcseql_node [1],
                            cseqh_rcseql_node [2], cseqh_rcseql_node [3], cseqh_rcseql_node [4],
                            cseqh_rcseql_node [5], cseqh_rcseql_node [6], cseqh_rcseql_node [7]);
                break;
            }
            else {
                snprintf_fn(buf.data(), buf.size_bytes(), "<INVALID>");
                return false;
            }
        } break;

        case data_type::DECIMALTYPE:
        case data_type::MONEYTYPE: {
            auto mt = field.as<tdsl::sqltypes::sql_money>();
            snprintf_fn(buf.data(), buf.size_bytes(), "%f", mt);
        } break;
        default:
            return false;
    }
    return true;
}

// --------------------------------------------------------------------------------

struct tdsl_field_to_string_fixture : public ::testing::Test {
    tdsl::tds_column_info ci{};
    uut_t field{};
    tdsl::char_span out{};

    inline void resize_out(tdsl::size_t n) {
        out_str_buf.resize(n);
        out = tdsl::char_span{out_str_buf.data(), out_str_buf.size()};
    }

    inline void update_field_data(void (*mutator)(std::vector<tdsl::uint8_t> &)) {
        mutator(field_data);
        field = uut_t{field_data.data(), field_data.size()};
    }

private:
    std::vector<tdsl::uint8_t> field_data{};
    std::vector<char> out_str_buf{};
};

// --------------------------------------------------------------------------------

// given when then
// WHEN("Field is null")

// GIVEN

// All of these are just syntatic sugar.
#define GIVEN
#define WHEN
#define THEN

TEST_F(tdsl_field_to_string_fixture, field_to_string_null) {
    GIVEN {
        resize_out(/*n=*/7);
        ci.type = tdsl::detail::e_tds_data_type::NULLTYPE;
    }
    WHEN {
        field_to_string(ci, field, out);
    }
    THEN {
        ASSERT_STREQ(out.data(), "<NULL>");
    }
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_to_string_fixture, field_to_string_bool_true) {

    GIVEN {
        resize_out(/*n=*/5);
        update_field_data(+[](std::vector<tdsl::uint8_t> & vec) {
            tdsl::int8_t a{1};
            vec.resize(sizeof(tdsl::int8_t));
            std::memcpy(vec.data(), &a, sizeof(tdsl::int8_t));
        });
        ci.type = tdsl::detail::e_tds_data_type::BITTYPE;
    }
    WHEN {
        field_to_string(ci, field, out);
    }
    THEN {
        ASSERT_STREQ(out.data(), "TRUE");
    }
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_to_string_fixture, field_to_string_bool_false) {
    GIVEN {
        resize_out(/*n=*/6);
        update_field_data(+[](std::vector<tdsl::uint8_t> & vec) {
            tdsl::int8_t a{0};
            vec.resize(sizeof(tdsl::int8_t));
            std::memcpy(vec.data(), &a, sizeof(tdsl::int8_t));
        });
        ci.type = tdsl::detail::e_tds_data_type::BITTYPE;
    }
    WHEN {
        field_to_string(ci, field, out);
    }
    THEN {
        ASSERT_STREQ(out.data(), "FALSE");
    }
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_to_string_fixture, field_to_string_int8) {
    GIVEN {
        resize_out(/*n=*/4);
        update_field_data(+[](std::vector<tdsl::uint8_t> & vec) {
            tdsl::int8_t a{127};
            vec.resize(sizeof(tdsl::int8_t));
            std::memcpy(vec.data(), &a, sizeof(tdsl::int8_t));
        });
        ci.type                 = tdsl::detail::e_tds_data_type::INT1TYPE;
        ci.typeprops.u8l.length = 1;
    }
    WHEN {
        field_to_string(ci, field, out);
    }
    THEN {
        ASSERT_STREQ(out.data(), "127");
    }
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_to_string_fixture, field_to_string_intn1) {
    GIVEN {
        resize_out(/*n=*/4);
        update_field_data(+[](std::vector<tdsl::uint8_t> & vec) {
            tdsl::int8_t a{127};
            vec.resize(sizeof(tdsl::int8_t));
            std::memcpy(vec.data(), &a, sizeof(tdsl::int8_t));
        });
        ci.type                 = tdsl::detail::e_tds_data_type::INTNTYPE;
        ci.typeprops.u8l.length = 1;
    }
    WHEN {
        field_to_string(ci, field, out);
    }
    THEN {
        ASSERT_STREQ(out.data(), "127");
    }
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_to_string_fixture, field_to_string_intn2) {
    GIVEN {
        resize_out(/*n=*/6);
        update_field_data(+[](std::vector<tdsl::uint8_t> & vec) {
            tdsl::int16_t a{32639};
            vec.resize(sizeof(tdsl::int16_t));
            std::memcpy(vec.data(), &a, sizeof(tdsl::int16_t));
        });
        ci.type                 = tdsl::detail::e_tds_data_type::INTNTYPE;
        ci.typeprops.u8l.length = 2;
    }
    WHEN {
        field_to_string(ci, field, out);
    }
    THEN {
        ASSERT_STREQ(out.data(), "32639");
    }
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_to_string_fixture, field_to_string_intn4) {
    GIVEN {
        resize_out(/*n=*/11);
        update_field_data(+[](std::vector<tdsl::uint8_t> & vec) {
            tdsl::int32_t a{2139062143};
            vec.resize(sizeof(tdsl::int32_t));
            std::memcpy(vec.data(), &a, sizeof(tdsl::int32_t));
        });
        ci.type                 = tdsl::detail::e_tds_data_type::INTNTYPE;
        ci.typeprops.u8l.length = 4;
    }
    WHEN {
        field_to_string(ci, field, out);
    }
    THEN {
        ASSERT_STREQ(out.data(), "2139062143");
    }
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_to_string_fixture, field_to_string_intn8) {
    GIVEN {
        resize_out(/*n=*/20);
        update_field_data(+[](std::vector<tdsl::uint8_t> & vec) {
            tdsl::int64_t a{9187201950435737471};
            vec.resize(sizeof(tdsl::int64_t));
            std::memcpy(vec.data(), &a, sizeof(tdsl::int64_t));
        });
        ci.type                 = tdsl::detail::e_tds_data_type::INTNTYPE;
        ci.typeprops.u8l.length = 8;
    }
    WHEN {
        field_to_string(ci, field, out);
    }
    THEN {
        ASSERT_STREQ(out.data(), "9187201950435737471");
    }
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_to_string_fixture, field_to_string_intn9) {
    GIVEN {
        resize_out(/*n=*/10);
        ci.type                 = tdsl::detail::e_tds_data_type::INTNTYPE;
        ci.typeprops.u8l.length = 9;
    }
    WHEN {
        field_to_string(ci, field, out);
    }
    THEN {
        ASSERT_STREQ(out.data(), "<INVALID>");
    }
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_to_string_fixture, field_to_string_guid) {
    // E2E86D76-8FE1-4728-AC9D-1CED7EFEA9D2
    GIVEN {
        resize_out(/*n=*/37);
        update_field_data(+[](std::vector<tdsl::uint8_t> & vec) {
            std::uint8_t buf [] = {0x76, 0x6D, 0xE8, 0xE2, 0xE1, 0x8F, 0x28, 0x47,
                                   0xAC, 0x9D, 0x1C, 0xED, 0x7E, 0xFE, 0xA9, 0xD2};
            vec.resize(sizeof(buf));
            std::memcpy(vec.data(), &buf, sizeof(buf));
        });
        ci.type = tdsl::detail::e_tds_data_type::GUIDTYPE;
    }
    WHEN {
        field_to_string(ci, field, out);
    }
    THEN {
        ASSERT_STRCASEEQ(out.data(), "E2E86D76-8FE1-4728-AC9D-1CED7EFEA9D2");
    }
}

// --------------------------------------------------------------------------------

TEST_F(tdsl_field_to_string_fixture, field_to_string_guid_false) {
    GIVEN {
        resize_out(/*n=*/37);
        update_field_data(+[](std::vector<tdsl::uint8_t> & vec) {
            std::uint8_t buf [] = {0x76, 0x6D, 0xE8, 0xE2, 0xE1, 0x8F, 0x28, 0x47,
                                   0xAC, 0x9D, 0x1C, 0xED, 0x7E, 0xFE, 0xA9};
            vec.resize(sizeof(buf));
            std::memcpy(vec.data(), &buf, sizeof(buf));
        });
        ci.type = tdsl::detail::e_tds_data_type::GUIDTYPE;
    }
    WHEN {
        field_to_string(ci, field, out);
    }
    THEN {
        ASSERT_STREQ(out.data(), "<INVALID>");
    }
}