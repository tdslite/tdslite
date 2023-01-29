2.2.5.6 Type Info Rule Definition
The TYPE_INFO rule applies to several messages used to describe column information. For columns of
fixed data length, the type is all that is required to determine the data length. For columns of a
variable-length type, TYPE_VARLEN defines the length of the data contained within the column, with
the following exceptions introduced in TDS 7.3:
DATE MUST NOT have a TYPE_VARLEN. The value is either 3 bytes or 0 bytes (null).
TIMENTYPE, DATETIME2NTYPE, and DATETIMEOFFSETNTYPE MUST NOT have a TYPE_VARLEN. The
lengths are determined by the SCALE as indicated in section 2.2.5.4.3.
PRECISION and SCALE MUST occur if the type is NUMERICTYPE, NUMERICNTYPE, DECIMALTYPE, or
DECIMALNTYPE.
SCALE (without PRECISION) MUST occur if the type is TIMENTYPE, DATETIME2NTYPE, or
DATETIMEOFFSETNTYPE (introduced in TDS 7.3). PRECISION MUST be less than or equal to decimal
38 and SCALE MUST be less than or equal to the precision value.
COLLATION occurs only if the type is BIGCHARTYPE, BIGVARCHARTYPE, TEXTTYPE, NTEXTTYPE,
NCHARTYPE, or NVARCHARTYPE.
UDT_INFO always occurs if the type is UDTTYPE.
XML_INFO always occurs if the type is XMLTYPE.
USHORTMAXLEN does not occur if PARTLENTYPE is XMLTYPE or UDTTYPE.

....
2.2.5.2.3 Data Type Dependent Data Streams
Some messages contain variable data types. The actual type of a given variable data type is
dependent on the type of the data being sent within the message as defined in the TYPE_INFO rule
(section 2.2.5.6).
For example, the RPCRequest message contains the TYPE_INFO and TYPE_VARBYTE rules. These two
rules contain data of a type that is dependent on the actual type used in the value of the
FIXEDLENTYPE or VARLENTYPE rules of the TYPE_INFO rule.
Data type-dependent data streams occur in three forms: integers, fixed and variable bytes, and
partially length-prefixed bytes.

....
2.2.2.5 Return Status
When a stored procedure is executed by the server, the server MUST return a status value. This is a
4-byte integer and is sent via the RETURNSTATUS token. A stored procedure execution is requested
through either an RPC Batch or a SQL Batch (section 2.2.1.4) message. For more details about
RETURNSTATUS, see section 2.2.7.18.

....
2.2.2.6 Return Parameters
The response format for execution of a stored procedure is identical regardless of whether the
request was sent as SQL Batch (section 2.2.1.4) or RPC Batch. It is always a tabular result-type
message.
If the procedure explicitly sends any data, then the message starts with a single token stream of rows,
informational messages, and error messages. This data is sent in the usual way.
When the RPC is invoked, some or all of its parameters are designated as output parameters. All
output parameters will have values returned from the server. For each output parameter, there is a
corresponding return value, sent via the RETURNVALUE token. The RETURNVALUE token data stream
is also used for sending back the value returned by a user-defined function (UD