/**
 * ____________________________________________________
 * Board-specific tdslite example for Arduino Uno WiFi
 - R2.
 *
 * @file   uno_wifi_r2.ino
 * @author mkg <hello@mkg.dev>
 * @date   14.07.2023
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#if !defined(ARDUINO_AVR_UNO_WIFI_REV2)
#warning "Current board is not Arduino Uno WiFi R2."
#endif

#include <WiFiNINA.h>

// --------------------------------------------------------------------------------
// Sketch options
// --------------------------------------------------------------------------------

#define SKETCH_ENABLE_SERIAL_OUTPUT                // comment out this to disable serial output
#define SKETCH_TDSL_NETBUF_SIZE     512 * 4        // tdslite network buffer size
#define SKETCH_TDSL_PACKET_SIZE     1400           // tds PDU size
#define SKETCH_WIFI_SSID            "<>"           // put your SSID (i.e. wifi name) here
#define SKETCH_WIFI_PASSWORD        "<>"           // wifi password
#define SKETCH_DB_IP_OR_HOSTNAME    "192.168.1.27" // database server IP
#define SKETCH_DB_NAME              "master"       // database to connect
#define SKETCH_DB_PORT              14333          // port number (default is 1433!)
#define SKETCH_DB_USERNAME          "sa"           // database user
#define SKETCH_DB_PASSWORD          "2022-tds-lite-test!" // database user's password

// --------------------------------------------------------------------------------
// Serial output
// --------------------------------------------------------------------------------

#ifdef SKETCH_ENABLE_SERIAL_OUTPUT

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

#else
#define SERIAL_PRINTF(FMTSTR, ...)
#define SERIAL_PRINTLNF(FMTSTR, ...)
#define SERIAL_PRINT_U16_AS_MB(U16SPAN)
#endif

#define TDSL_DEBUG_PRINT(FMTSTR, ...)       SERIAL_PRINTF(FMTSTR, ##__VA_ARGS__)
#define TDSL_DEBUG_PRINTLN(FMTSTR, ...)     SERIAL_PRINTLNF(FMTSTR, ##__VA_ARGS__)
#define TDSL_DEBUG_PRINT_U16_AS_MB(U16SPAN) SERIAL_PRINT_U16_AS_MB(U16SPAN)

#include <tdslite.h>

inline bool init_serial() {
#ifdef SKETCH_ENABLE_SERIAL_OUTPUT
    Serial.begin(115200);
    while (!Serial)
        ;
#endif
    return true;
}

// --------------------------------------------------------------------------------
// WiFi
// --------------------------------------------------------------------------------

inline void scan_aps() {
    SERIAL_PRINTLNF("... scanning networks ...");
    const auto ssid_count = WiFi.scanNetworks();
    if (ssid_count == -1) {
        SERIAL_PRINTLNF("... cannot get a WiFi connection! ...");
        return;
    }
    SERIAL_PRINTLNF("... found %d network(s) ...", ssid_count);
    for (auto i = 0; i < ssid_count; i++) {
        SERIAL_PRINTLNF("... [%d]: %s (%ld dBm, encryption %d) ...", i, WiFi.SSID(i), WiFi.RSSI(i),
                        WiFi.encryptionType(i));
    }
}

// --------------------------------------------------------------------------------

inline bool wifi_setup() {
    SERIAL_PRINTLNF("... wifi setup ...");

    const char ssid []     = SKETCH_WIFI_SSID;
    const char password [] = SKETCH_WIFI_PASSWORD;
    // WiFi.mode(WIFI_STA); // Optional
    int status             = WL_IDLE_STATUS; // the WiFi radio's status

    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        // don't continue
        while (true)
            ;
    }
    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        Serial.println("Please upgrade the firmware");
    }

    scan_aps();

    while (not(status == WL_CONNECTED)) {
        status = WiFi.begin(ssid, password);
        SERIAL_PRINTLNF("... waiting for WiFi connection ...");
        Serial.print(status);
        delay(15000);
    };
    SERIAL_PRINTLNF("Connected to the WiFi network `%s`, IP address is: %d.%d.%d.%d", ssid,
                    WiFi.localIP() [0], WiFi.localIP() [1], WiFi.localIP() [2], WiFi.localIP() [3]);
    return true;
}

// --------------------------------------------------------------------------------
// user callbacks
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
    SERIAL_PRINTLNF("");
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
    (void) u;
    (void) colmd;

    SERIAL_PRINTF("row: ");
    for (const auto & field : row) {
        SERIAL_PRINTF("%d\t", field.as<tdsl::uint32_t>());
    }
    SERIAL_PRINTLNF("");
}

// --------------------------------------------------------------------------------
// tdslite

// The network buffer
tdsl::uint8_t net_buf [SKETCH_TDSL_NETBUF_SIZE] = {};
tdsl::arduino_driver<WiFiClient> driver{net_buf};

// --------------------------------------------------------------------------------

void tdslite_database_init() noexcept {
    SERIAL_PRINTLNF("... init database begin...");
    const auto r = driver.execute_query("CREATE TABLE #hello_world(a int, b int, c varchar(255));");
    (void) r;
    SERIAL_PRINTLNF("... init database end, result `%d`...", r);
}

// --------------------------------------------------------------------------------

bool tdslite_setup() noexcept {
    SERIAL_PRINTLNF("... init tdslite ...");
    decltype(driver)::connection_parameters params;
    // Server's hostname or IP address.
    params.server_name = SKETCH_DB_IP_OR_HOSTNAME; // WL
    //  SQL server port number
    params.port        = SKETCH_DB_PORT;
    // Login user
    params.user_name   = SKETCH_DB_USERNAME;
    // Login user password
    params.password    = SKETCH_DB_PASSWORD;
    // Client name(optional)
    params.client_name = "arduino uno test sketch";
    // App name(optional)
    params.app_name    = "sketch";
    // Database name(optional)
    params.db_name     = SKETCH_DB_NAME;
    // TDS packet size
    // Recommendation: Half of the network buffer.
    params.packet_size = {SKETCH_TDSL_PACKET_SIZE};
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

// --------------------------------------------------------------------------------
// main sketch routines
// --------------------------------------------------------------------------------

inline void rgb_led_set_color(int r, int g, int b) noexcept {
    WiFiDrv::analogWrite(25, r);
    WiFiDrv::analogWrite(26, g);
    WiFiDrv::analogWrite(27, b);
}

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
    // set onboard RGB led to a random color
    // in order to indicate activity
    rgb_led_set_color(random(0, 255), random(0, 255), random(0, 255));
    delay(250);
}
