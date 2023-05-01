/**
 * ____________________________________________________
 * Example code for illustrating how to use info/error
 * callback function to receive info/error messages
 * sent by the connected SQL server.
 *
 * @file   04-info-callback.ino
 * @author mkg <hello@mkg.dev>
 * @date   01.05.2023
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <Ethernet.h>
#include <tdslite.h>

// --------------------------------------------------------------------------------
// serial output related stuff

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

static inline void serial_setup() {
#ifdef SKETCH_ENABLE_SERIAL_OUTPUT
    Serial.begin(115200);
    while (!Serial)
        ;
#endif
}

// --------------------------------------------------------------------------------
// ethernet

static inline void ethernet_setup() {
    SERIAL_PRINTLNF("Setup!");
    byte mac [] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEE};
    IPAddress ip(192, 168, 1, 244);
    Ethernet.begin(mac, ip);

    // Check if *any* ethernet hardware is detected.
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        SERIAL_PRINTLNF("Error: No ethernet hardware detected!");
        // Ethernet shield not detected
        while (true) {
            delay(1000);
        }
    }
}

// --------------------------------------------------------------------------------
// tdslite

tdsl::uint8_t net_buf [768] = {};
tdsl::arduino_driver<EthernetClient> driver{net_buf};

/**
 * Prints INFO/ERROR messages from SQL server to serial output.
 *
 * @param [in] token INFO/ERROR token
 */
static void info_callback(void *, const tdsl::tds_info_token & token) noexcept {
    SERIAL_PRINTF("%c: [%d/%d/%d@%d] --> ", (token.is_info() ? 'I' : 'E'), token.number,
                  token.state, token.class_, token.line_number);
    SERIAL_PRINT_U16_AS_MB(token.msgtext);
    SERIAL_PRINTLNF("");
}

// --------------------------------------------------------------------------------

/**
 * Connect to the SQL server at 192.168.1.45:14333
 * with tdslite library. The function also sets the info callback
 * function to `info_callback`.
 */
static inline void tdslite_setup() {
    decltype(driver)::progmem_connection_parameters params;
    params.server_name = TDSL_PMEMSTR("192.168.1.45"); // WL
    params.port        = 14333;                        // default port is 1433
    params.user_name   = TDSL_PMEMSTR("sa");
    params.password    = TDSL_PMEMSTR("2022-tds-lite-test!");
    params.db_name     = TDSL_PMEMSTR("master");
    params.packet_size = {512};

    // Set a callback function that'll process the
    // info/error messages sent by the server
    driver.set_info_callback(info_callback);

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
}

// --------------------------------------------------------------------------------

/**
 * example loop function.
 *
 * The function declares one static integer, increments it and
 * checks whether the value is even or not. If the value is even,
 * the code proceeds to execute a query that prints an INFO message
 * containing current UTC date from SQL server. Otherwise, the code
 * tries to select a column named `foo` from nonexistent table `bar`,
 * which causes SQL server to send back an error message.
 *
 * The messages will be printed to the serial output.
 */
static inline void tdslite_loop() {
    static int i = 0;

    if (i++ % 2 == 0) {
        // Execute a valid query:
        // (which will cause SQL server to send an INFO message)
        driver.execute_query(TDSL_PMEMSTR("PRINT getutcdate();"));
        // this'll also generate an info message:
        driver.execute_query(TDSL_PMEMSTR("SET LANGUAGE English;"));
    }
    else {
        // Execute an invalid query
        // (which will cause SQL server to send an ERROR message)
        driver.execute_query(TDSL_PMEMSTR("select foo from #bar;"));
        // this'll also generate an error message:
        driver.execute_query(TDSL_PMEMSTR("RAISERROR (15600, -1, -1, 'my_special_procedure')"));
        // ... and also this:
        driver.execute_query(TDSL_PMEMSTR("foo bar"));
    }
    delay(1000);
}

// --------------------------------------------------------------------------------

void setup() {
    serial_setup();
    ethernet_setup();
    tdslite_setup();
}

void loop() {
    tdslite_loop();
}