 TDSL_MESSAGE_TOKEN_TYPE_LIST_PROLOGUE 
        // A notification of an environment change (for example, database, language, and so on).
        TDSL_MESSAGE_TOKEN_TYPE_DECL(envchange, 0xe3) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Used to send an error message to the client.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(error, 0xaa) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Used to send a complete row of total data, where the data format is provided by the ALTMETADATA
        // token.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(altrow, 0xd3) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Describes the column information in browse mode [MSDN-BROWSE], sp_cursoropen, and
        // sp_cursorfetch.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(colinfo, 0xa5) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Describes the result set for interpretation of following ROW data streams.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(colmetadata, 0x81) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Introduced in TDS 7.4, the DATACLASSIFICATION token SHOULD<42> describe the data
        // classification of the query result set.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(dataclassification, 0xa3) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Indicates the completion status of a SQL statement.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(done, 0xfd) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Indicates the completion status of a SQL statement within a stored procedure.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(doneinproc, 0xff) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Indicates the completion status of a stored procedure. This is also generated for stored procedures
        // executed through SQL statements.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(doneproc, 0xfe) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Introduced in TDS 7.4, FEATUREEXTACK is used to send an optional acknowledge message to the
        // client for features that are defined in FeatureExt. The token stream is sent only along with the
        // LOGINACK in a Login Response message.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(featureextack, 0xae) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Introduced in TDS 7.4, federated authentication information is returned to the client to be used for
        // generating a Federated Authentication Token during the login process. This token MUST be the only
        // token in a Federated Authentication Information message and MUST NOT be included in any other
        // message type.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(fedauthinfo, 0xee) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Used to send an information message to the client.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(info, 0xab) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Used to send a response to a login request (LOGIN7) to the client.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(loginack, 0xad) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
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
        TDSL_MESSAGE_TOKEN_TYPE_DECL(nbcrow, 0xd2) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Used to inform the client where in the client's SQL text buffer a particular keyword occurs.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(offset, 0x78) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Used to inform the client by which columns the data is ordered.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(order, 0xa9) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Used to send the status value of an RPC to the client. The server also uses this token to send the
        // result status value of a T-SQL EXEC query.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(returnstatus, 0x79) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Used to send the return value of an RPC to the client. When an RPC is executed, the associated
        // parameters might be defined as input or output (or "return") parameters. This token is used to send a
        // description of the return parameter to the client. This token is also used to describe the value returned
        // by a UDF when executed as an RPC.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(returnvalue, 0xac) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Used to send a complete row, as defined by the COLMETADATA token, to the client.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(row, 0xd1) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Used to send session state data to the client. The data format defined here can also be used to send
        // session state data for session recovery during login and login response.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(sessionstate, 0xe4) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // The SSPI token returned during the login process.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(sspi, 0xed) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Used to send the table name to the client only when in browser mode or from sp_cursoropen.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(tabname, 0xa4) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Used to send a complete table valued parameter (TVP) row, as defined by the TVP_COLMETADATA
        // token from client to server.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(tvp_row, 0x01) TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
        // Describes the data type, length, and name of column data that result from a SQL statement that
        // generates totals.
        TDSL_MESSAGE_TOKEN_TYPE_DECL(altmetadata, 0x88)
TDSL_MESSAGE_TOKEN_TYPE_LIST_EPILOGUE

// Auto-undef

#undef TDSL_MESSAGE_TOKEN_TYPE_LIST_PROLOGUE
#undef TDSL_MESSAGE_TOKEN_TYPE_DECL
#undef TDSL_MESSAGE_TOKEN_TYPE_LIST_DELIM
#undef TDSL_MESSAGE_TOKEN_TYPE_LIST_EPILOGUE