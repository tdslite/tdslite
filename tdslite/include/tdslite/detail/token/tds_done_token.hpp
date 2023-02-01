/**
 * ____________________________________________________
 *
 * @file   tds_done_token.hpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   23.05.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#ifndef TDSL_DETAIL_TOKEN_TDS_DONE_TOKEN_HPP
#define TDSL_DETAIL_TOKEN_TDS_DONE_TOKEN_HPP

#include <tdslite/util/tdsl_inttypes.hpp>

namespace tdsl {

    /**
     * TDS DONE token (0xFD)
     */
    struct tds_done_token {
        // The Status field MUST be a bitwise 'OR' of the following:
        // * 0x00:  DONE_FINAL. This DONE is the final DONE in the request.
        // * 0x1:   DONE_MORE. This DONE message is not the final DONE message in the response.
        //          Subsequent data streams to follow.
        // * 0x2:   DONE_ERROR. An error occurred on the current SQL statement. A preceding ERROR
        //          token SHOULD be sent when this bit is set.
        // * 0x4:   DONE_INXACT. A transaction is in progress.<43>
        // * 0x10:  DONE_COUNT. The DoneRowCount value is valid. This is used to distinguish between
        //          a valid value of 0 for DoneRowCount or just an initialized variable.
        // * 0x20:  DONE_ATTN. The DONE message is a server acknowledgement of a client
        //          ATTENTION message.
        // * 0x100: DONE_SRVERROR. Used in place of DONE_ERROR when an error occurred on the
        //          current SQL statement, which is severe enough to require the result set, if any,
        //          to be discarded
        tdsl::uint16_t status         = {0};
        // The token of the current SQL statement. The token value is provided and controlled by the
        // application layer, which utilizes TDS. The TDS layer does not evaluate the value.
        tdsl::uint16_t curcmd         = {0};
        // The count of rows that were affected by the SQL statement. The value of DoneRowCount is
        // valid if the value of Status includes DONE_COUNT.
        tdsl::uint32_t done_row_count = {0};
    };
} // namespace tdsl

#endif