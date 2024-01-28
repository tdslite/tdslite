/**
 * ____________________________________________________
 * Internal test sketch for WiFi-enabled boards like
 * ESP and Arduino WiFi
 *
 * Known to be compatible with the following boards:
 *
 * - NodeMCU-32S (ESP32S)
 * - NodeMCU-v3 (ESP8266)
 * - Arduino Uno WiFi R2
 *
 * @file   esp.cpp
 * @author mkg <hello@mkg.dev>
 * @date   14.07.2023
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#if defined ESP8266
#include <ESP8266WiFi.h>
#define SKETCH_TDSL_NETBUF_SIZE 512 * 16
#define SKETCH_TDSL_PACKET_SIZE 1400 // ~tcp segment size

#elif defined ESP32
#include <WiFi.h>
#include <esp_task_wdt.h>

#define SKETCH_TDSL_NETBUF_SIZE 512 * 16
#define SKETCH_TDSL_PACKET_SIZE 1400 // ~tcp segment size

#elif defined ARDUINO_AVR_UNO_WIFI_REV2
#include <WiFiNINA.h>

#define SKETCH_TDSL_NETBUF_SIZE 512 * 4
#define SKETCH_TDSL_PACKET_SIZE 1400 // ~tcp segment size
#else
#error "Architecture unrecognized by this code."
#endif

/**
 * Print a formatted string to serial output.
 *
 * @param [in] FMTSTR Format string
 * @param [in] ...    Format arguments
 *
 * The format string will be stored in program memory in order
 * to save space.
 */
#define SERIAL_PRINTF(FMTSTR, ...)                                                                 \
    [&]() {                                                                                        \
        static_assert(sizeof(FMTSTR) <= 255, "Format string cannot be greater than 255");          \
        /* Save format string into program */                                                      \
        /* memory to save flash space */                                                           \
        static const char __fmtpm [] PROGMEM = FMTSTR;                                             \
        char buf [128]                       = {};                                                 \
        snprintf_P(buf, sizeof(buf), __fmtpm, ##__VA_ARGS__);                                      \
        Serial.print(buf);                                                                         \
    }()

#define SERIAL_PRINTLNF(FMTSTR, ...)                                                               \
    SERIAL_PRINTF(FMTSTR, ##__VA_ARGS__);                                                          \
    Serial.println("")

#define SERIAL_PRINT_U16_AS_MB(U16SPAN)                                                            \
    [](tdsl::u16char_view v) {                                                                     \
        for (const auto ch : v) {                                                                  \
            Serial.print(static_cast<char>(ch));                                                   \
        }                                                                                          \
    }(U16SPAN)

#define TDSL_DEBUG_PRINT(FMTSTR, ...)       SERIAL_PRINTF(FMTSTR, ##__VA_ARGS__)
#define TDSL_DEBUG_PRINTLN(FMTSTR, ...)     SERIAL_PRINTLNF(FMTSTR, ##__VA_ARGS__)
#define TDSL_DEBUG_PRINT_U16_AS_MB(U16SPAN) SERIAL_PRINT_U16_AS_MB(U16SPAN)

#ifndef SKETCH_NO_TDSLITE

#include <tdslite.h>

// --------------------------------------------------------------------------------

/**
 * Prints INFO/ERROR messages from SQL server to stdout.
 *
 * @param [in] token INFO/ERROR token
 */
static void info_callback(void *, const tdsl::tds_info_token & token) noexcept {
    SERIAL_PRINTF("%c: [%d/%d/%d@%d] --> ", (token.is_info() ? 'I' : 'E'), token.number,
                  token.state, token.class_, token.line_number);
    SERIAL_PRINT_U16_AS_MB(token.msgtext);
    SERIAL_PRINTF("\n");
}

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
    // Feed the dog so he won't bite us

#if defined ESP8266
    ESP.wdtFeed();
#elif defined ESP32
    esp_task_wdt_reset();
#endif

    SERIAL_PRINTF("row: ");

    // TODO: implement a visitor for this?
    for (const auto & field : row) {
        SERIAL_PRINTF("%d\t", field.as<tdsl::uint32_t>());
    }
    SERIAL_PRINTF("\n");
}

// The network buffer
tdsl::uint8_t net_buf [SKETCH_TDSL_NETBUF_SIZE] = {};
tdsl::arduino_driver<WiFiClient> driver{net_buf};

// --------------------------------------------------------------------------------

void tdslite_database_init() noexcept {
    SERIAL_PRINTLNF("... init database begin...");
    const auto r = driver.execute_query("CREATE TABLE #hello_world(a int, b int, c varchar(255));");
    (void) r;
    SERIAL_PRINTLNF("... init database end, result `%d`...", r.affected_rows);
}

// --------------------------------------------------------------------------------

bool tdslite_setup() noexcept {
    SERIAL_PRINTLNF("... init tdslite ...");
    decltype(driver)::connection_parameters params;
    // Server's hostname or IP address.
    params.server_name         = "192.168.1.27"; // WL
    // params.server_name = PSTR("192.168.1.45"); // WS
    //  SQL server port number
    params.port                = 14333;
    // Login user
    params.user_name           = "sa";
    // Login user password
    params.password            = "2022-tds-lite-test!";
    // Client name(optional)
    params.client_name         = "arduino mega";
    // App name(optional)
    params.app_name            = "sketch";
    // Database name(optional)
    params.db_name             = "master";
    // TDS packet size
    // Recommendation: Half of the network buffer.
    params.packet_size         = {SKETCH_TDSL_PACKET_SIZE};
    // How many times the driver should attempt to connect to the server
    params.conn_retry_count    = 5;
    // Delay between each connection attempt (milliseconds)
    params.conn_retry_delay_ms = 2000;
    driver.set_info_callback(&info_callback, nullptr);
    auto cr = driver.connect(params);
    if (not(decltype(driver)::e_driver_error_code::success == cr)) {
        SERIAL_PRINTLNF("... tdslite init failed, connection failed %d ...",
                        static_cast<tdsl::uint32_t>(cr));
        return false;
    }

    tdslite_database_init();
    SERIAL_PRINTLNF("... init tdslite end...");
    return true;
}

// --------------------------------------------------------------------------------

inline void tdslite_loop() noexcept {
    static int i = 0;
    driver.execute_query(
        "INSERT INTO #hello_world VALUES(1,2, "
        "'Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor "
        "incididunt ut labore et dolore magna aliqua. Semper viverra nam libero justo laoreet sit "
        "amet. Fringilla ut morbi tincidunt augue interdum velit.')");
    if (i++ % 10 == 0) {
        const auto result = driver.execute_query("SELECT * FROM #hello_world;", row_callback);
        SERIAL_PRINTLNF(">> Report: row count [%d] <<", result.affected_rows);
    }
}

#else

inline void tdslite_loop() {}

inline void initDatabase() {}

#endif

// --------------------------------------------------------------------------------

inline bool init_serial() {
    Serial.begin(115200);
    while (!Serial)
        ;
    return true;
}

// --------------------------------------------------------------------------------

inline bool wifi_setup() {
    SERIAL_PRINTLNF("... wifi setup ...");
    const char * ssid     = "<>";
    const char * password = "<>";
    WiFi.begin(ssid, password);
    while (not(WiFi.status() == WL_CONNECTED)) {
        SERIAL_PRINTLNF("... waiting for WiFi connection ...");
        delay(1000);
    };
    SERIAL_PRINTLNF("Connected to the WiFi network `%s`, IP address is: %d.%d.%d.%d", ssid,
                    WiFi.localIP() [0], WiFi.localIP() [1], WiFi.localIP() [2], WiFi.localIP() [3]);
    return true;
}

// --------------------------------------------------------------------------------

void setup() {
    bool r = {false};
    r      = init_serial();
    r      = r && wifi_setup();
    r      = r && tdslite_setup();
    if (not r) {
        SERIAL_PRINTLNF("... setup failed ...");
        for (;;) {
            delay(1000);
        }
    }
    SERIAL_PRINTLNF("--- setup finished ---");
}

// --------------------------------------------------------------------------------

void loop() {
    tdslite_loop();
    delay(250);
}