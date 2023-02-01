/**
 * ____________________________________________________
 *
 * @file   tdsl_message_token_type.hpp
 * @author Mustafa K. GILOR <mustafagilor@gmail.com>
 * @date   25.04.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TDS_MESSAGE_TOKEN_TYPE_HPP
#define TDSL_DETAIL_TDS_MESSAGE_TOKEN_TYPE_HPP

#include <tdslite/util/tdsl_inttypes.hpp>
#include <tdslite/util/tdsl_macrodef.hpp>

namespace tdsl { namespace detail {

    enum class e_tds_message_token_type : tdsl::uint8_t

    {
#define TDSL_MESSAGE_TOKEN_TYPE_DECL(NAME, VALUE) NAME = VALUE
#define TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM        ,
#include <tdslite/detail/tdsl_message_token_type.inc>
    };

    /**
     * Translate message token type value @type to string
     *
     * @param [in] type Token type
     * @return Token type as string if @p type has a corresponding string representation,
     * "UNDEFINED" otherwise.
     */
    static inline TDSL_NODISCARD TDSL_CXX14_CONSTEXPR const char *
    message_token_type_to_str(e_tds_message_token_type type) {
        switch (type) {
#define TDSL_MESSAGE_TOKEN_TYPE_DECL(NAME, VALUE)                                                  \
    case e_tds_message_token_type::NAME:                                                           \
        return #NAME "(" #VALUE ")";
#define TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM ;

#include <tdslite/detail/tdsl_message_token_type.inc>
        }

        return "UNDEFINED";
    }

}} // namespace tdsl::detail

#endif