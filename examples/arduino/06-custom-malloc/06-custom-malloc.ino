/**
 * ____________________________________________________
 * Example code for illustrating how to use non-default
 * user provided memory management functions.
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
 * @file   06-custom-malloc.ino
 * @author mkg <me@mustafagilor.com>
 * @date   26.02.2023
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <Ethernet.h>

// Define this to disable default memory management functions.
#define TDSL_DISABLE_DEFAULT_ALLOCATOR

#include <tdslite.h>
#include <stdlib.h>

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

tdsl::uint8_t net_buf [768]                 = {};
tdsl::arduino_driver<EthernetClient> driver = {net_buf};
byte mac []                                 = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEB};
IPAddress ip                                = {192, 168, 1, 246};

// --------------------------------------------------------------------------------

/**
 * Custom malloc to trace memory allocations
 * over serial output
 */
inline void * my_malloc(unsigned long amount) noexcept {
    auto alloc = ::malloc(amount);
    SERIAL_PRINTLNF("alloc req %lu, result %p", amount, alloc);
    return alloc;
}

// --------------------------------------------------------------------------------

inline void my_free(void * ptr) noexcept {
    SERIAL_PRINTLNF("free req %p", ptr);
    return ::free(ptr);
}

// --------------------------------------------------------------------------------

/**
 * The setup function initializes Serial output,
 * Ethernet interface, tdslite library and then
 * the database tables.
 */
void setup() {
#ifdef SKETCH_ENABLE_SERIAL_OUTPUT
    Serial.begin(115200);
    while (!Serial)
        ;
#endif

    //////////////////////////
    // Initialize ethernet interface
    //////////////////////////

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

    // set malloc/free functions for tdslite
    tdsl::tdslite_malloc_free(my_malloc, my_free);

    decltype(driver)::progmem_connection_parameters params;
    // Server's hostname or IP address.
    params.server_name = TDSL_PMEMSTR("192.168.1.45"); // WL
    // SQL server port number
    params.port        = 14333; // default port is 1433
    // SQL server login user
    params.user_name   = TDSL_PMEMSTR("sa");
    // SQL server login user password
    params.password    = TDSL_PMEMSTR("2022-tds-lite-test!");
    // TDS packet size
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
 * The loop function executes INSERT query every
 * 1 second, and SELECT query every 10 seconds.
 */
void loop() {

    // Your queries goes here.

    auto query{TDSL_PMEMSTR("INSERT INTO #example_table VALUES('test', 1)")};

    SERIAL_PRINTF("Executing query: ");
    SERIAL_PRINTLNF_PROGMEM(query.raw_data());
    // `ra` is rows_affected
    auto result = driver.execute_query(query);
    SERIAL_PRINTLNF("Rows affected: %d", result.affected_rows);

    delay(1000);
}