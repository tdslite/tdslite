/**
 * _________________________________________________
 *
 * @file   tdsl_token_type.hpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

#pragma once

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>

namespace tdsl { namespace detail {

#define TDSLITE_MESSAGE_TOKEN_TYPE_LIST_PROLOGUE                                                                                           \
    enum class e_tds_message_token_type : tdsl::uint8_t                                                                                    \
    {
#define TDSLITE_MESSAGE_TOKEN_TYPE_DECL(NAME, VALUE) NAME = VALUE
#define TDSLITE_MESSAGE_TOKEN_TYPE_LIST_DELIM        ,
#define TDSLITE_MESSAGE_TOKEN_TYPE_LIST_EPILOGUE                                                                                           \
    }                                                                                                                                      \
    ;

#include <tdslite/detail/tdsl_message_token_type.inc>

    /**
     * Translate message token type value @type to string
     *
     * @param [in] type Token type
     * @return Token type as string if @p type has a corresponding string representation, "UNDEFINED" otherwise.
     */
    inline static TDSLITE_CXX14_CONSTEXPR const char * message_token_type_to_str(e_tds_message_token_type type) {
#define TDSLITE_MESSAGE_TOKEN_TYPE_LIST_PROLOGUE switch (type) {
#define TDSLITE_MESSAGE_TOKEN_TYPE_DECL(NAME, VALUE)                                                                                       \
    case e_tds_message_token_type::NAME:                                                                                                   \
        return #NAME "(" #VALUE ")";
#define TDSLITE_MESSAGE_TOKEN_TYPE_LIST_DELIM ;
#define TDSLITE_MESSAGE_TOKEN_TYPE_LIST_EPILOGUE                                                                                           \
    }                                                                                                                                      \
    ;
#include <tdslite/detail/tdsl_message_token_type.inc>

        return "UNDEFINED";
    }

}} // namespace tdsl::detail