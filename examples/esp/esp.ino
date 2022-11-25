#if defined ESP8266
#include <ESP8266WiFi.h>
#elif defined ESP32
#include <WiFi.h>
#else
#error "Architecture unrecognized by this code."
#endif

#define SKETCH_TDSL_NETBUF_SIZE 512 * 16
#define SKETCH_TDSL_PACKET_SIZE 1400 // ~tcp segment size

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

/**
 * Handle row data coming from tdsl driver
 *
 * @param [in] u user pointer (table_context)
 * @param [in] colmd Column metadata
 * @param [in] row Row information
 */
static void row_callback(void * u, const tdsl::tds_colmetadata_token & colmd,
                         const tdsl::tdsl_row & row) {
    SERIAL_PRINTF("row: ");
    for (const auto & field : row) {
        SERIAL_PRINTF("%d\t", field.as<tdsl::uint32_t>());
    }
    SERIAL_PRINTLNF("");
}

// The network buffer
tdsl::uint8_t net_buf [SKETCH_TDSL_NETBUF_SIZE] = {};
tdsl::arduino_driver<WiFiClient> driver{net_buf};

void tdslite_database_init() {
    SERIAL_PRINTLNF("... init database begin...");
    const int r = driver.execute_query("CREATE TABLE #hello_world(a int, b int, c varchar(255));");
    (void) r;
    SERIAL_PRINTLNF("... init database end, result `%d`...", r);
}

void tdslite_setup() {
    SERIAL_PRINTLNF("... init tdslite ...");
    tdsl::arduino_driver<WiFiClient>::connection_parameters params;
    // Server's hostname or IP address.
    params.server_name = "192.168.1.22"; // WL
    // params.server_name = PSTR("192.168.1.45"); // WS
    //  SQL server port number
    params.port        = 14333;
    // Login user
    params.user_name   = "sa";
    // Login user password
    params.password    = "2022-tds-lite-test!";
    // Client name(optional)
    params.client_name = "arduino mega";
    // App name(optional)
    params.app_name    = "sketch";
    // Database name(optional)
    params.db_name     = "master";
    // TDS packet size
    // Recommendation: Half of the network buffer.
    params.packet_size = {SKETCH_TDSL_PACKET_SIZE};
    driver.set_info_callback(nullptr, &info_callback);
    driver.connect(params);
    tdslite_database_init();
    SERIAL_PRINTLNF("... init tdslite end...");
}

inline void tdslite_loop() {
    static int i = 0;
    driver.execute_query("INSERT INTO #hello_world VALUES(1,2, "
                         "'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                         "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')");
    if (i++ % 10 == 0) {
        const auto row_count =
            driver.execute_query("SELECT * FROM #hello_world;", nullptr, row_callback);
        SERIAL_PRINTLNF(">> Report: row count [%d] <<", row_count);
    }
}

#else

inline void tdslite_loop() {}

inline void initDatabase() {}

#endif

inline void init_serial() {
    Serial.begin(112500);
    while (!Serial)
        ;
}

inline void wifi_setup() {
    const char * ssid     = "<>";
    const char * password = "<>";
    WiFi.mode(WIFI_STA); // Optional
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        SERIAL_PRINTLNF("... waiting for WiFi connection ...");
        delay(1000);
    };
    SERIAL_PRINTLNF("Connected to the WiFi network `%s`, IP address is: %s", ssid,
                    WiFi.localIP().toString().c_str());
}

void setup() {
    init_serial();
    wifi_setup();
    tdslite_setup();
}

void loop() {
    tdslite_loop();
    delay(250);
}