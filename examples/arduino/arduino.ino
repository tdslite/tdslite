// arduino-specific debug printers.
#define SKETCH_ENABLE_WATCHDOG
// #define SKETCH_TDSL_DEBUG_LOG
#define SKETCH_SERIAL_OUTPUT
//  #define SKETCH_USE_DHCP
//  #define SKETCH_NO_TDSLITE
#define SKETCH_TDSL_NETBUF_SIZE 512 + 256
#define SKETCH_TDSL_PACKET_SIZE 512

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
            char buf [64]                        = {};                                             \
            snprintf_P(buf, sizeof(buf), __fmtpm, ##__VA_ARGS__);                                  \
            Serial.print(buf);                                                                     \
        }()

    #define SERIAL_PRINTLNF(FMTSTR, ...)                                                           \
        SERIAL_PRINTF(FMTSTR, ##__VA_ARGS__);                                                      \
        Serial.println("");                                                                        \
        Serial.flush()

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

#if defined SKETCH_ENABLE_WATCHDOG
    #include "ApplicationMonitor.h"
Watchdog::CApplicationMonitor ApplicationMonitor;

#endif

#include <Ethernet.h>

#ifndef SKETCH_NO_TDSLITE
    #define TDSL_DISABLE_DEFAULT_ALLOCATOR 1
    #include <tdslite.h>

extern unsigned int __heap_start;
extern void * __brkval;

/*
 * The free list structure as maintained by the
 * avr-libc memory allocation routines.
 */
struct __freelist {
    size_t sz;
    struct __freelist * nx;
};

/* The head of the free list structure */
extern struct __freelist * __flp;

/* Calculates the size of the free list */
static int freeListSize() {
    struct __freelist * current;
    int total = 0;

    for (current = __flp; current; current = current->nx) {
        total += 2; /* Add two bytes for the memory block's header  */
        total += (int) current->sz;
    }

    return total;
}

static int freeRam() {
    int free_memory;

    if ((int) __brkval == 0) {
        free_memory = ((int) &free_memory) - ((int) &__heap_start);
    }
    else {
        free_memory = ((int) &free_memory) - ((int) __brkval);
        free_memory += freeListSize();
    }
    return free_memory;
}

/**
 * Prints INFO/ERROR messages from SQL server to stdout.
 *
 * @param [in] token INFO/ERROR token
 */
static void info_callback(void *, const tdsl::tds_info_token & token) noexcept {
    SERIAL_PRINTLNF("m_inf %d", freeRam());
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
    digitalWrite(5, HIGH);
    ApplicationMonitor.IAmAlive();
    SERIAL_PRINTF("row: ");
    for (const auto & field : row) {
        SERIAL_PRINTF("%d\t", field.as<tdsl::uint32_t>());
    }
    SERIAL_PRINTLNF("");
    digitalWrite(5, LOW);
}

// The network buffer
tdsl::uint8_t net_buf [SKETCH_TDSL_NETBUF_SIZE] = {};
tdsl::arduino_driver<EthernetClient> driver{net_buf};

inline void * my_malloc(unsigned long amount) noexcept {
    auto alloc = ::malloc(amount);
    SERIAL_PRINTLNF("alloc req %lu, result %p, rem: %d", amount, alloc, freeRam());
    return alloc;
}

// --------------------------------------------------------------------------------

inline void my_free(void * ptr) noexcept {
    SERIAL_PRINTLNF("free req %p", ptr);
    return ::free(ptr);
}

void initTdsliteDriver() {
    SERIAL_PRINTLNF("... init tdslite ...");
    tdsl::tdslite_malloc_free(my_malloc, my_free);
    tdsl::arduino_driver<EthernetClient>::progmem_connection_parameters params;
    // Server's hostname or IP address.
    params.server_name = TDSL_PMEMSTR("192.168.1.22"); // WL
    // params.server_name = TDSL_PMEMSTR("192.168.1.45"); // WS
    //  SQL server port number
    params.port        = 14333;
    // Login user
    params.user_name   = TDSL_PMEMSTR("sa");
    // Login user password
    params.password    = TDSL_PMEMSTR("2022-tds-lite-test!");
    // Client name(optional)
    params.client_name = TDSL_PMEMSTR("arduino mega");
    // App name(optional)
    params.app_name    = TDSL_PMEMSTR("sketch");
    // Database name(optional)
    params.db_name     = TDSL_PMEMSTR("master");
    // TDS packet size
    // Recommendation: Half of the network buffer.
    params.packet_size = {SKETCH_TDSL_PACKET_SIZE};
    driver.set_info_callback(nullptr, &info_callback);
    auto result = driver.connect(params);
    SERIAL_PRINTLNF("... init tdslite end, %d...", result);
}

void initDatabase() {
    const int r = driver.execute_query(TDSL_PMEMSTR("CREATE TABLE #hello_world(a int, b int);"));
    (void) r;
    SERIAL_PRINTLNF("... init database %d...", r);
    SERIAL_PRINTLNF("... init database end...");
}

inline void tdslite_loop() {
    static int i = 0;
    driver.execute_query(TDSL_PMEMSTR("INSERT INTO #hello_world VALUES(1,2)"));
    if (i++ % 10 == 0) {
        const auto row_count = driver.execute_query(TDSL_PMEMSTR("SELECT * FROM #hello_world;"),
                                                    nullptr, row_callback);
        SERIAL_PRINTLNF(">> Report: row count [%d], free RAM [%d] <<", row_count, freeRam());
        SERIAL_PRINTLNF("%d", freeRam());
    }
}

#else

inline void tdslite_loop() {}

inline void initDatabase() {}

#endif

/**
 * Initialize serial port for logging
 */
inline void initSerialPort() {
#if defined SKETCH_SERIAL_OUTPUT
    Serial.begin(112500);
    while (!Serial)
        ;
#endif
    SERIAL_PRINTLNF("m_isp %d", freeRam());
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
    SERIAL_PRINTLNF("m_ies %d", freeRam());
}

#if defined SKETCH_ENABLE_WATCHDOG
inline void initWatchdog() {
    ApplicationMonitor.Dump(Serial);
    ApplicationMonitor.EnableWatchdog(Watchdog::CApplicationMonitor::Timeout_8s);
}

#else
inline void initWatchdog() {}
#endif

inline void initLEDs() {
    pinMode(3, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(7, OUTPUT);
    pinMode(9, OUTPUT);
    digitalWrite(3, HIGH);
    digitalWrite(5, HIGH);
    digitalWrite(7, HIGH);
    digitalWrite(9, HIGH);
}

void setup() {
    initLEDs();
    initSerialPort();
    SERIAL_PRINTLNF("sizeof ptr(%d), size_t(%d)", sizeof(tdsl::uint8_t *), sizeof(tdsl::size_t));
    SERIAL_PRINTLNF("sizeof tdsl::span<unsigned char>(%d), tdsl::arduino_driver(%d)",
                    sizeof(tdsl::span<unsigned char>),
                    sizeof(tdsl::arduino_driver<EthernetClient>));
    initEthernetShield();
    initTdsliteDriver();
    initDatabase();
    SERIAL_PRINTLNF("... setup complete ...");
    SERIAL_PRINTLNF("m_setup %lu", freeRam());
    initWatchdog();
}

void loop() {
    ApplicationMonitor.IAmAlive();
    tdslite_loop();
#ifdef SKETCH_USE_DHCP
    Ethernet.maintain();
#endif
    digitalWrite(3, HIGH);
    delay(250);
    digitalWrite(3, LOW);
}