/**
 * _________________________________________________
 *
 * @file   ut_tdsl_data_type.cpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   09.01.2023
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#include <tdslite/detail/tdsl_data_type.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <utility>
#include <tuple>

using dtype = tdsl::detail::e_tds_data_type;
using stype = tdsl::detail::e_tds_data_size_type;

// --------------------------------------------------------------------------------

struct dtype_fixture : public ::testing::Test {};

// --------------------------------------------------------------------------------

struct dtype_fixed_len_fixture
    : public dtype_fixture,
      public testing::WithParamInterface<std::tuple<dtype, std::size_t, dtype>> {};

// --------------------------------------------------------------------------------

struct dtype_var_precision_len_fixture
    : public dtype_fixture,
      public testing::WithParamInterface<std::pair<dtype, std::size_t>> {};

// --------------------------------------------------------------------------------

struct dtype_var_u8_len_fixture : public dtype_fixture,
                                  public testing::WithParamInterface<dtype> {};

// --------------------------------------------------------------------------------

struct dtype_var_u16_len_fixture : public dtype_fixture,
                                   public testing::WithParamInterface<std::pair<dtype, bool>> {};

// --------------------------------------------------------------------------------

struct dtype_var_u32_len_fixture : public dtype_fixture,
                                   public testing::WithParamInterface<std::pair<dtype, bool>> {};

// --------------------------------------------------------------------------------

struct dtype_str_fixture : public dtype_fixture,
                           public testing::WithParamInterface<std::pair<dtype, const char *>> {};

// --------------------------------------------------------------------------------

TEST_P(dtype_fixed_len_fixture, props) {
    const auto & prop = tdsl::detail::get_data_type_props(std::get<0>(GetParam()));
    ASSERT_EQ(stype::fixed, prop.size_type);

    EXPECT_EQ(std::get<1>(GetParam()), prop.length.fixed);
    EXPECT_EQ(std::get<2>(GetParam()), prop.corresponding_varsize_type);
    EXPECT_FALSE(prop.is_variable_size());
    EXPECT_FALSE(prop.flags.has_collation);
    EXPECT_FALSE(prop.flags.has_precision);
    EXPECT_FALSE(prop.flags.has_table_name);
    EXPECT_FALSE(prop.flags.has_textptr);
    EXPECT_FALSE(prop.flags.maxlen_represents_null);
    EXPECT_FALSE(prop.flags.zero_represents_null);
}

// --------------------------------------------------------------------------------

TEST_P(dtype_var_precision_len_fixture, props) {
    const auto & prop = tdsl::detail::get_data_type_props(std::get<0>(GetParam()));
    EXPECT_EQ(std::get<1>(GetParam()), prop.length.variable.length_size);
    EXPECT_EQ(std::get<0>(GetParam()), prop.corresponding_varsize_type);
    ASSERT_EQ(stype::var_precision, prop.size_type);
    EXPECT_TRUE(prop.is_variable_size());
    EXPECT_TRUE(prop.flags.has_precision);
    EXPECT_FALSE(prop.flags.has_collation);
    EXPECT_FALSE(prop.flags.has_table_name);
    EXPECT_FALSE(prop.flags.has_textptr);
    EXPECT_FALSE(prop.flags.maxlen_represents_null);
    EXPECT_FALSE(prop.flags.zero_represents_null);
}

// --------------------------------------------------------------------------------

TEST_P(dtype_var_u8_len_fixture, props) {
    const auto & prop = tdsl::detail::get_data_type_props(GetParam());
    EXPECT_EQ(sizeof(tdsl::uint8_t), prop.length.variable.length_size);
    EXPECT_EQ(GetParam(), prop.corresponding_varsize_type);
    ASSERT_EQ(stype::var_u8, prop.size_type);
    EXPECT_TRUE(prop.is_variable_size());
    EXPECT_FALSE(prop.flags.has_precision);
    EXPECT_FALSE(prop.flags.has_collation);
    EXPECT_FALSE(prop.flags.has_table_name);
    EXPECT_FALSE(prop.flags.has_textptr);
    EXPECT_FALSE(prop.flags.maxlen_represents_null);
    EXPECT_TRUE(prop.flags.zero_represents_null);
}

// --------------------------------------------------------------------------------

TEST_P(dtype_var_u16_len_fixture, props) {
    const auto & prop = tdsl::detail::get_data_type_props(std::get<0>(GetParam()));
    EXPECT_EQ(sizeof(tdsl::uint16_t), prop.length.variable.length_size);
    EXPECT_EQ(std::get<0>(GetParam()), prop.corresponding_varsize_type);
    EXPECT_EQ(std::get<1>(GetParam()), prop.flags.has_collation);
    ASSERT_EQ(stype::var_u16, prop.size_type);
    EXPECT_TRUE(prop.is_variable_size());
    EXPECT_TRUE(prop.flags.maxlen_represents_null);
    EXPECT_FALSE(prop.flags.has_precision);
    EXPECT_FALSE(prop.flags.has_table_name);
    EXPECT_FALSE(prop.flags.has_textptr);
    EXPECT_FALSE(prop.flags.zero_represents_null);
}

// --------------------------------------------------------------------------------

TEST_P(dtype_var_u32_len_fixture, props) {
    const auto & prop = tdsl::detail::get_data_type_props(std::get<0>(GetParam()));
    EXPECT_EQ(sizeof(tdsl::uint32_t), prop.length.variable.length_size);
    EXPECT_EQ(std::get<0>(GetParam()), prop.corresponding_varsize_type);
    EXPECT_EQ(std::get<1>(GetParam()), prop.flags.has_collation);
    ASSERT_EQ(stype::var_u32, prop.size_type);
    EXPECT_TRUE(prop.is_variable_size());
    EXPECT_TRUE(prop.flags.maxlen_represents_null);
    EXPECT_TRUE(prop.flags.has_table_name);
    EXPECT_TRUE(prop.flags.has_textptr);
    EXPECT_FALSE(prop.flags.has_precision);
    EXPECT_FALSE(prop.flags.zero_represents_null);
}

// --------------------------------------------------------------------------------
TEST_P(dtype_str_fixture, to_str) {
    const auto & str = tdsl::detail::data_type_to_str(std::get<0>(GetParam()));
    ASSERT_STREQ(std::get<1>(GetParam()), str);
}

// --------------------------------------------------------------------------------

INSTANTIATE_TEST_SUITE_P(
    cf, dtype_fixed_len_fixture,
    testing::Values(std::make_tuple(dtype::NULLTYPE, 0, dtype::NULLTYPE),
                    std::make_tuple(dtype::INT1TYPE, 1, dtype::INTNTYPE),
                    std::make_tuple(dtype::BITTYPE, 1, dtype::BITNTYPE),
                    std::make_tuple(dtype::INT2TYPE, 2, dtype::INTNTYPE),
                    std::make_tuple(dtype::INT4TYPE, 4, dtype::INTNTYPE),
                    std::make_tuple(dtype::INT8TYPE, 8, dtype::INTNTYPE),
                    std::make_tuple(dtype::DATETIM4TYPE, 4, dtype::DATETIMNTYPE),
                    std::make_tuple(dtype::FLT4TYPE, 4, dtype::FLTNTYPE),
                    std::make_tuple(dtype::DATETIMETYPE, 8, dtype::DATETIMNTYPE),
                    std::make_tuple(dtype::FLT8TYPE, 8, dtype::FLTNTYPE),
                    std::make_tuple(dtype::MONEYTYPE, 8, dtype::MONEYNTYPE),
                    std::make_tuple(dtype::MONEY4TYPE, 4, dtype::MONEYNTYPE)));

// --------------------------------------------------------------------------------

INSTANTIATE_TEST_SUITE_P(cf, dtype_var_precision_len_fixture,
                         testing::Values(std::make_pair(dtype::DECIMALNTYPE, 2),
                                         std::make_pair(dtype::NUMERICNTYPE, 2)));

// --------------------------------------------------------------------------------

INSTANTIATE_TEST_SUITE_P(cf, dtype_var_u8_len_fixture,
                         testing::Values(dtype::GUIDTYPE, dtype::INTNTYPE, dtype::BITNTYPE,
                                         dtype::FLTNTYPE, dtype::MONEYNTYPE, dtype::DATETIMNTYPE));

// --------------------------------------------------------------------------------

INSTANTIATE_TEST_SUITE_P(cf, dtype_var_u16_len_fixture,
                         testing::Values(std::make_pair(dtype::BIGCHARTYPE, true),
                                         std::make_pair(dtype::BIGVARCHRTYPE, true),
                                         std::make_pair(dtype::NVARCHARTYPE, true),
                                         std::make_pair(dtype::NCHARTYPE, true),
                                         std::make_pair(dtype::BIGBINARYTYPE, false),
                                         std::make_pair(dtype::BIGVARBINTYPE, false)));

// --------------------------------------------------------------------------------

INSTANTIATE_TEST_SUITE_P(cf, dtype_var_u32_len_fixture,
                         testing::Values(std::make_pair(dtype::NTEXTTYPE, true),
                                         std::make_pair(dtype::TEXTTYPE, true),
                                         std::make_pair(dtype::IMAGETYPE, false)));

// --------------------------------------------------------------------------------

INSTANTIATE_TEST_SUITE_P(
    tostr, dtype_str_fixture,
    testing::Values(std::make_pair(dtype::NULLTYPE, "NULLTYPE(0x1)"),
                    std::make_pair(dtype::INT1TYPE, "INT1TYPE(0x30)"),
                    std::make_pair(dtype::BITTYPE, "BITTYPE(0x32)"),
                    std::make_pair(dtype::INT2TYPE, "INT2TYPE(0x34)"),
                    std::make_pair(dtype::INT4TYPE, "INT4TYPE(0x38)"),
                    std::make_pair(dtype::DATETIM4TYPE, "DATETIM4TYPE(0x3A)"),
                    std::make_pair(dtype::FLT4TYPE, "FLT4TYPE(0x3B)"),
                    std::make_pair(dtype::MONEYTYPE, "MONEYTYPE(0x3C)"),
                    std::make_pair(dtype::DATETIMETYPE, "DATETIMETYPE(0x3D)"),
                    std::make_pair(dtype::FLT8TYPE, "FLT8TYPE(0x3E)"),
                    std::make_pair(dtype::MONEY4TYPE, "MONEY4TYPE(0x7A)"),
                    std::make_pair(dtype::INT8TYPE, "INT8TYPE(0x7F)"),
                    std::make_pair(dtype::GUIDTYPE, "GUIDTYPE(0x24)"),
                    std::make_pair(dtype::INTNTYPE, "INTNTYPE(0x26)"),
                    std::make_pair(dtype::DECIMALTYPE, "DECIMALTYPE(0x37)"),
                    std::make_pair(dtype::NUMERICTYPE, "NUMERICTYPE(0x3F)"),
                    std::make_pair(dtype::BITNTYPE, "BITNTYPE(0x68)"),
                    std::make_pair(dtype::DECIMALNTYPE, "DECIMALNTYPE(0x6A)"),
                    std::make_pair(dtype::NUMERICNTYPE, "NUMERICNTYPE(0x6C)"),
                    std::make_pair(dtype::FLTNTYPE, "FLTNTYPE(0x6D)"),
                    std::make_pair(dtype::MONEYNTYPE, "MONEYNTYPE(0x6E)"),
                    std::make_pair(dtype::DATETIMNTYPE, "DATETIMNTYPE(0x6F)"),
                    std::make_pair(dtype::BIGVARBINTYPE, "BIGVARBINTYPE(0xA5)"),
                    std::make_pair(dtype::BIGVARCHRTYPE, "BIGVARCHRTYPE(0xA7)"),
                    std::make_pair(dtype::BIGBINARYTYPE, "BIGBINARYTYPE(0xAD)"),
                    std::make_pair(dtype::BIGCHARTYPE, "BIGCHARTYPE(0xAF)"),
                    std::make_pair(dtype::NVARCHARTYPE, "NVARCHARTYPE(0xE7)"),
                    std::make_pair(dtype::NCHARTYPE, "NCHARTYPE(0xEF)"),
                    std::make_pair(dtype::TEXTTYPE, "TEXTTYPE(0x23)"),
                    std::make_pair(dtype::IMAGETYPE, "IMAGETYPE(0x22)"),
                    std::make_pair(dtype::NTEXTTYPE, "NTEXTTYPE(0x63)")));

// --------------------------------------------------------------------------------

struct dtype_mcmd_fixture : public ::testing::Test {

    virtual void SetUp() override {
        props.size_type    = tdsl::detail::e_tds_data_size_type::fixed;
        props.length.fixed = 1;
    }

    tdsl::detail::tds_data_type_properties props{};
};

// --------------------------------------------------------------------------------

TEST_F(dtype_mcmd_fixture, min_colmetadata_size) {
    EXPECT_EQ(2, props.min_colmetadata_size());
}

// --------------------------------------------------------------------------------

TEST_F(dtype_mcmd_fixture, min_colmetadata_size_var) {
    props.size_type                   = tdsl::detail::e_tds_data_size_type::var_u8;
    props.length.variable.length_size = 1;
    EXPECT_EQ(2, props.min_colmetadata_size());
}

// --------------------------------------------------------------------------------

TEST_F(dtype_mcmd_fixture, min_colmetadata_size_coll) {
    props.flags.has_collation = true;
    EXPECT_EQ(7, props.min_colmetadata_size());
}

// --------------------------------------------------------------------------------

TEST_F(dtype_mcmd_fixture, min_colmetadata_size_precision) {
    props.flags.has_precision = true;
    EXPECT_EQ(4, props.min_colmetadata_size());
}

// --------------------------------------------------------------------------------

TEST_F(dtype_mcmd_fixture, min_colmetadata_size_tablename) {
    props.flags.has_table_name = true;
    EXPECT_EQ(4, props.min_colmetadata_size());
}

// --------------------------------------------------------------------------------

TEST_F(dtype_mcmd_fixture, min_colmetadata_size_cpt) {
    props.flags.has_collation  = true;
    props.flags.has_precision  = true;
    props.flags.has_table_name = true;
    EXPECT_EQ(11, props.min_colmetadata_size());
}

// --------------------------------------------------------------------------------

TEST_F(dtype_mcmd_fixture, is_variable_size_fixed) {
    props.size_type = tdsl::detail::e_tds_data_size_type::fixed;
    ASSERT_FALSE(props.is_variable_size());
}

// --------------------------------------------------------------------------------

TEST_F(dtype_mcmd_fixture, is_variable_size_vu8) {
    props.size_type = tdsl::detail::e_tds_data_size_type::var_u8;
    ASSERT_TRUE(props.is_variable_size());
}

// --------------------------------------------------------------------------------

TEST_F(dtype_mcmd_fixture, is_variable_size_vu16) {
    props.size_type = tdsl::detail::e_tds_data_size_type::var_u16;
    ASSERT_TRUE(props.is_variable_size());
}

// --------------------------------------------------------------------------------

TEST_F(dtype_mcmd_fixture, is_variable_size_vu32) {
    props.size_type = tdsl::detail::e_tds_data_size_type::var_u32;
    ASSERT_TRUE(props.is_variable_size());
}

// --------------------------------------------------------------------------------

TEST_F(dtype_mcmd_fixture, is_variable_size_vp) {
    props.size_type = tdsl::detail::e_tds_data_size_type::var_precision;
    ASSERT_TRUE(props.is_variable_size());
}