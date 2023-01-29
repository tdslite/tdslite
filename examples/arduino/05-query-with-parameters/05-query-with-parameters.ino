/**
 * ____________________________________________________
 * Example code for illustrating how to bind parameters
 * to parameterized SQL query, a.k.a. `how to use
 * execute_rpc`
 *
 * Prerequisites:
 * - A board with an Ethernet shield
 * - A running SQL server (*)
 * - tdslite library installed via Library Manager
 *
 * (*) The development environment container has an embedded
 *     SQL server exposed to host at port 14333.
 *
 * Tested with Arduino Uno w/ Ethernet shield.
 *
 * @file   05-query-with-parameters.ino
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   22.01.2023
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <Ethernet.h>
#include <tdslite.h>

// Serial output uses ~175 bytes of SRAM space
// and ~840 bytes of program memory.
#define SKETCH_ENABLE_SERIAL_OUTPUT

#if defined SKETCH_ENABLE_SERIAL_OUTPUT

#define SERIAL_PRINTF_PROGMEM(FMTPM, ...)                                                          \
    char buf [64] = {};                                                                            \
    snprintf_P(buf, sizeof(buf), FMTPM, ##__VA_ARGS__);                                            \
    Serial.print(buf);

#define SERIAL_PRINTF(FMTSTR, ...)                                                                 \
    [&]() {                                                                                        \
        /* Save format string into program */                                                      \
        /* memory to save flash space */                                                           \
        static const char __fmtpm [] PROGMEM = FMTSTR;                                             \
        SERIAL_PRINTF_PROGMEM(__fmtpm, ##__VA_ARGS__)                                              \
    }()

#define SERIAL_PRINTLNF_PROGMEM(FMTPM, ...)                                                        \
    SERIAL_PRINTF_PROGMEM(FMTPM, ##__VA_ARGS__)                                                    \
    Serial.println("");                                                                            \
    Serial.flush()

#define SERIAL_PRINTLNF(FMTSTR, ...)                                                               \
    SERIAL_PRINTF(FMTSTR, ##__VA_ARGS__);                                                          \
    Serial.println("");                                                                            \
    Serial.flush()

#define SERIAL_PRINT_U16_AS_MB(U16SPAN)                                                            \
    [](tdsl::u16char_view v) {                                                                     \
        for (const auto ch : v) {                                                                  \
            Serial.print(static_cast<char>(ch));                                                   \
        }                                                                                          \
    }(U16SPAN)
#else
#define SERIAL_PRINTF_PROGMEM(FMTPM, ...)
#define SERIAL_PRINTF(FMTSTR, ...)
#define SERIAL_PRINTLNF_PROGMEM(FMTPM, ...)
#define SERIAL_PRINTLNF(FMTSTR, ...)
#define SERIAL_PRINT_U16_AS_MB(U16SPAN)
#endif

// --------------------------------------------------------------------------------

/**
 * The network buffer.
 *
 * The library will use this buffer for network I/O.
 *
 * The buffer must be at least 512 bytes in size.
 * In order to have some headroom for fragmentation
 * it is recommended to allocate 768 bytes at least.
 *
 * The actual size need for network buffer is depends
 * on your use case.
 *
 */
tdsl::uint8_t net_buf [768] = {};

// --------------------------------------------------------------------------------

/**
 * The tdslite driver object.
 *
 * tdsl::arduino_driver class is a templated type
 * where the template argument is the TCP client
 * implementation compatible with Arduino's
 * EthernetClient interface.
 *
 * The client will be initialized internally.
 */
tdsl::arduino_driver<EthernetClient> driver{net_buf};

// --------------------------------------------------------------------------------

/**
 * MAC address for the ethernet interface
 */
byte mac [] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEB};

// --------------------------------------------------------------------------------

/**
 * IP address for the ethernet interface.
 * Change it according to your network address space
 */
IPAddress ip(192, 168, 1, 246);

// --------------------------------------------------------------------------------

/**
 * The setup function initializes Serial output,
 * Ethernet interface, tdslite library and then
 * the database tables.
 */
void setup() {
#ifdef SKETCH_ENABLE_SERIAL_OUTPUT
    Serial.begin(112500);
    while (!Serial)
        ;
#endif

    //////////////////////////
    // Initialize ethernet interface
    //////////////////////////

    // The reason we're not using DHCP here is, this is
    // a minimal example with absolute minimum space
    // requirements, so the code can work on boards with
    // tight memory constraints (i.e. Arduino Uno/Nano)
    //
    // DHCP requires UDP, UDP requires extra space.
    // We're not using DHCP here to save some program
    // memory and SRAM space.

    SERIAL_PRINTLNF("Initializing ethernet interface");
    // Try to configure ethernet interface
    // with given MAC and IP
    Ethernet.begin(mac, ip);

    // Check if *any* ethernet hardware is detected.
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        SERIAL_PRINTLNF("Error: No ethernet hardware detected!");
        // Ethernet shield not detected
        while (true) {
            delay(1000);
        }
    }

    //////////////////////////
    // Initialize tdslite
    //////////////////////////

    // Declare a connection parameters struct. We will fill this struct
    // with the details of the SQL server/database we want to connect to.
    // We're using progmem_connection_parameters here, because we want to
    // store database connection parameters in program memory in order to
    // save some precious SRAM space.
    decltype(driver)::progmem_connection_parameters params;
    // Server's hostname or IP address.
    params.server_name = TDSL_PMEMSTR("192.168.1.45"); // WL
    // SQL server port number
    params.port        = 14333; // default port is 1433
    // SQL server login user
    params.user_name   = TDSL_PMEMSTR("sa");
    // SQL server login user password
    params.password    = TDSL_PMEMSTR("2022-tds-lite-test!");
    // Client name(optional)
    params.client_name = TDSL_PMEMSTR("arduino mega");
    // App name(optional)
    params.app_name    = TDSL_PMEMSTR("sketch");
    // Database name(optional)
    params.db_name     = TDSL_PMEMSTR("master");
    // TDS packet size
    // Recommendation: Half of the network buffer.
    // This is the PDU size that TDS protocol will use.
    // Given that the example has 768 bytes of network buffer space,
    // we set this to 512 to allow some headroom for fragmentation.
    params.packet_size = {512};

    SERIAL_PRINTLNF("Initializing tdslite");

    // Try to connect with given parameters. If connection succeeds,
    // the `result` will be e_driver_error_code::success. Otherwise,
    // the connection attempt has failed.
    auto result = driver.connect(params);

    if (not(decltype(driver)::e_driver_error_code::success == result)) {
        SERIAL_PRINTLNF("Error: Database connection failed!");
        // Database connection failed.
        while (true) {
            delay(1000);
        }
    }

    //////////////////////////
    // Initialize the database
    //////////////////////////

    driver.execute_query(TDSL_PMEMSTR("CREATE TABLE #example_table(a varchar(12), b int);"));
}

// --------------------------------------------------------------------------------

/**
 * How many times the loop function has
 * been invoked.
 */
static int loop_counter = {0};

// --------------------------------------------------------------------------------

/**
 * Handle row data coming from tdsl driver
 *
 * @param [in] u user pointer (table_context)
 * @param [in] colmd Column metadata
 * @param [in] row Row information
 */
static void row_callback(void * u, const tdsl::tds_colmetadata_token & colmd,
                         const tdsl::tdsl_row & row) {
    SERIAL_PRINTLNF("row: %.4s %d", row [0].as<tdsl::char_view>().data(),
                    row [1].as<tdsl::int32_t>());
}

// --------------------------------------------------------------------------------

/**
 * The loop function executes INSERT query every
 * 1 second, and SELECT query every 10 seconds.
 */
void loop() {
    // Your queries goes here.

    auto query{TDSL_PMEMSTR("INSERT INTO #example_table VALUES('test', 1)")};

    SERIAL_PRINTF("Executing query: ");
    SERIAL_PRINTLNF_PROGMEM(query.raw_data());
    // `ra` is rows_affected
    int ra = driver.execute_query(query);
    SERIAL_PRINTLNF("Result: %d", ra);

    // Execute SELECT query on every tenth loop.
    if (0 == (loop_counter % 10)) {
        auto query{TDSL_PMEMSTR("SELECT * FROM #example_table WHERE a = @p0 AND b = @p1")};

        SERIAL_PRINTF("Executing query: ");
        SERIAL_PRINTLNF_PROGMEM(query.raw_data());
        // We're using the row
        tdsl::sql_parameter_varchar a{"test"};
        tdsl::sql_parameter_int b{1};
        tdsl::sql_parameter_binding params []{a, b};
        driver.execute_rpc(query, params, tdsl::rpc_mode::executesql, row_callback);
        // SERIAL_PRINTLNF("Result: %d", ra);
    }

    delay(1000);

    // Increment the loop counter.
    loop_counter++;
}