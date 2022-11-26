// arduino-specific debug printers.

#define SKETCH_TDSL_DEBUG_LOG
// #define SKETCH_SERIAL_OUTPUT
//  #define SKETCH_USE_DHCP
//  #define SKETCH_NO_TDSLITE
#define SKETCH_TDSL_NETBUF_SIZE 512 * 2
#define SKETCH_TDSL_PACKET_SIZE SKETCH_TDSL_NETBUF_SIZE / 2

#if defined SKETCH_SERIAL_OUTPUT

    /**
     * Print a formatted string to serial output.
     *
     * @param [in] FMTSTR Format string
     * @param [in] ...    Format arguments
     *
     * The format string will be stored in program memory in order
     * to save space.
     */
    #define SERIAL_PRINTF(FMTSTR, ...)                                                             \
        [&]() {                                                                                    \
            static_assert(sizeof(FMTSTR) <= 255, "Format string cannot be greater than 255");      \
            /* Save format string into program */                                                  \
            /* memory to save flash space */                                                       \
            static const char __fmtpm [] PROGMEM = FMTSTR;                                         \
            char buf [128]                       = {};                                             \
            snprintf_P(buf, sizeof(buf), __fmtpm, ##__VA_ARGS__);                                  \
            Serial.print(buf);                                                                     \
        }()

    #define SERIAL_PRINTLNF(FMTSTR, ...)                                                           \
        SERIAL_PRINTF(FMTSTR, ##__VA_ARGS__);                                                      \
        Serial.println("")

    #define SERIAL_PRINT_U16_AS_MB(U16SPAN)                                                        \
        [](tdsl::u16char_view v) {                                                                 \
            for (const auto ch : v) {                                                              \
                Serial.print(static_cast<char>(ch));                                               \
            }                                                                                      \
        }(U16SPAN)

    #if defined SKETCH_TDSL_DEBUG_LOG
        #define TDSL_DEBUG_PRINT(FMTSTR, ...)       SERIAL_PRINTF(FMTSTR, ##__VA_ARGS__)
        #define TDSL_DEBUG_PRINTLN(FMTSTR, ...)     SERIAL_PRINTLNF(FMTSTR, ##__VA_ARGS__)
        #define TDSL_DEBUG_PRINT_U16_AS_MB(U16SPAN) SERIAL_PRINT_U16_AS_MB(U16SPAN)
    #endif

#else

    #define SERIAL_PRINTF(FMTSTR, ...)
    #define SERIAL_PRINTLNF(FMTSTR, ...)
    #define SERIAL_PRINT_U16_AS_MB(U16SPAN)
#endif

#include <Ethernet.h>

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
tdsl::arduino_driver<EthernetClient> driver{net_buf};

void initTdsliteDriver() {
    SERIAL_PRINTLNF("... init tdslite ...");
    tdsl::arduino_driver<EthernetClient>::connection_parameters params;
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
    SERIAL_PRINTLNF("... init tdslite end...");
}

void initDatabase() {
    const int r = driver.execute_query("CREATE TABLE #hello_world(a int, b int);");
    (void) r;
    SERIAL_PRINTLNF("... init database %d...", r);
    SERIAL_PRINTLNF("... init database end...");
}

inline void tdslite_loop() {
    static int i = 0;
    driver.execute_query("INSERT INTO #hello_world VALUES(1,2)");
    if (i++ % 10 == 0) {
        const auto row_count =
            driver.execute_query("SELECT * FROM #hello_world;", nullptr, row_callback);
        SERIAL_PRINTLNF(">> Report: row count [%d], free RAM [%d] <<", row_count, freeMemory());
    }
}

#else

inline void tdslite_loop() {}

inline void initDatabase() {}

#endif

int freeMemory() {

#ifdef __arm__
    // should use uinstd.h to define sbrk but Due causes a conflict
    extern "C" char * sbrk(int incr);
#else  // __ARM__
    extern char * __brkval;
#endif // __arm__

    char top;
#ifdef __arm__
    return &top - reinterpret_cast<char *>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
    return &top - __brkval;
#else  // __arm__
    return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif // __arm__
}

/**
 * Initialize serial port for logging
 */
inline void initSerialPort() {
#if defined SKETCH_SERIAL_OUTPUT
    Serial.begin(112500);
    while (!Serial)
        ;
#endif
}

/**
 * Initialize the ethernet shield with DHCP
 *
 * MUST be done before initializing tdslite.
 */
inline void initEthernetShield() {
    SERIAL_PRINTLNF("... init eth ...");
    byte mac [] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
#if defined SKETCH_USE_DHCP
    if (Ethernet.begin(mac) == 0) {
        SERIAL_PRINTLNF("Failed to configure Ethernet using DHCP");
        // Check for Ethernet hardware present
        if (Ethernet.hardwareStatus() == EthernetNoHardware) {
            SERIAL_PRINTLNF(
                "Ethernet shield was not found.  Sorry, can't run without hardware. :(");
            while (true) {
                delay(1); // do nothing, no point running without Ethernet hardware
            }
        }
        if (Ethernet.linkStatus() == LinkOFF) {
            SERIAL_PRINTLNF("Ethernet cable is not connected.");
        }
        // try to congifure using IP address instead of DHCP:
        // Ethernet.begin(mac, ip, myDns);
    }
    else {
        SERIAL_PRINTLNF("  DHCP assigned IP %s", Ethernet.localIP());
    }
#else
    IPAddress ip(192, 168, 1, 241);
    Ethernet.begin(mac, ip);
#endif
}

void setup() {
    initSerialPort();
    initEthernetShield();
    initTdsliteDriver();
    initDatabase();
    SERIAL_PRINTLNF("sizeof ptr(%d), size_t(%d), tdsl::span<unsigned char>(%d)",
                    sizeof(tdsl::uint8_t *), sizeof(tdsl::size_t),
                    sizeof(tdsl::span<unsigned char>));
    SERIAL_PRINTLNF("... setup complete ...");
}

void loop() {
    tdslite_loop();
#ifdef SKETCH_USE_DHCP
    Ethernet.maintain();
#endif
    delay(250);
}