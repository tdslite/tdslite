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

namespace tdsl { namespace detail {

    /**
     * Message token types, as per described in [MS-TDS]
     */
    enum class e_tds_message_token_type : tdsl::uint8_t
    {
        // A notification of an environment change (for example, database, language, and so on).
        envchange          = 0xe3,
        // Used to send an error message to the client.
        error              = 0xaa,
        // Used to send a complete row of total data, where the data format is provided by the ALTMETADATA
        // token.
        altrow             = 0xd3,
        // Describes the column information in browse mode [MSDN-BROWSE], sp_cursoropen, and
        // sp_cursorfetch.
        colinfo            = 0xa5,
        // Describes the result set for interpretation of following ROW data streams.
        colmetadata        = 0x81,
        // Introduced in TDS 7.4, the DATACLASSIFICATION token SHOULD<42> describe the data
        // classification of the query result set.
        dataclassification = 0xa3,
        // Indicates the completion status of a SQL statement.
        done               = 0xfd,
        // Indicates the completion status of a SQL statement within a stored procedure.
        doneinproc         = 0xff,
        // Indicates the completion status of a stored procedure. This is also generated for stored procedures
        // executed through SQL statements.
        doneproc           = 0xfe,
        // Introduced in TDS 7.4, FEATUREEXTACK is used to send an optional acknowledge message to the
        // client for features that are defined in FeatureExt. The token stream is sent only along with the
        // LOGINACK in a Login Response message.
        featureextack      = 0xae,
        // Introduced in TDS 7.4, federated authentication information is returned to the client to be used for
        // generating a Federated Authentication Token during the login process. This token MUST be the only
        // token in a Federated Authentication Information message and MUST NOT be included in any other
        // message type.
        fedauthinfo        = 0xee,
        // Used to send an information message to the client.
        info               = 0xab,
        // Used to send a response to a login request (LOGIN7) to the client.
        loginack           = 0xad,
        // NBCROW, introduced in TDS 7.3.B, is used to send a row as defined by the COLMETADATA token to
        // the client with null bitmap compression. Null bitmap compression is implemented by using a single bit
        // to specify whether the column is null or not null and also by removing all null column values from the
        // row. Removing the null column values (which can be up to 8 bytes per null instance) from the row
        // provides the compression. The null bitmap contains one bit for each column defined in COLMETADATA.
        // In the null bitmap, a bit value of 1 means that the column is null and therefore not present in the row,
        // and a bit value of 0 means that the column is not null and is present in the row. The null bitmap is
        // always rounded up to the nearest multiple of 8 bits, so there might be 1 to 7 leftover reserved bits at
        // the end of the null bitmap in the last byte of the null bitmap. NBCROW is only used by TDS result set
        // streams from server to client. NBCROW MUST NOT be used in BulkLoadBCP streams. NBCROW MUST
        // NOT be used in TVP row streams.
        nbcrow             = 0xd2,
        // Used to inform the client where in the client's SQL text buffer a particular keyword occurs.
        offset             = 0x78,
        // Used to inform the client by which columns the data is ordered.
        order              = 0xa9,
        // Used to send the status value of an RPC to the client. The server also uses this token to send the
        // result status value of a T-SQL EXEC query.
        returnstatus       = 0x79,
        // Used to send the return value of an RPC to the client. When an RPC is executed, the associated
        // parameters might be defined as input or output (or "return") parameters. This token is used to send a
        // description of the return parameter to the client. This token is also used to describe the value returned
        // by a UDF when executed as an RPC.
        returnvalue        = 0xac,
        // Used to send a complete row, as defined by the COLMETADATA token, to the client.
        row                = 0xd1,
        // Used to send session state data to the client. The data format defined here can also be used to send
        // session state data for session recovery during login and login response.
        sessionstate       = 0xe4,
        // The SSPI token returned during the login process.
        sspi               = 0xed,
        // Used to send the table name to the client only when in browser mode or from sp_cursoropen.
        tabname            = 0xa4,
        // Used to send a complete table valued parameter (TVP) row, as defined by the TVP_COLMETADATA
        // token from client to server.
        tvp_row            = 0x01,
        // Describes the data type, length, and name of column data that result from a SQL statement that
        // generates totals.
        altmetadata        = 0x88,
    };

}} // namespace tdsl::detail