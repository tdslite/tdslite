#define TDSL_DEBUG_PRINT(A, ...)   Serial.print(A)
#define TDSL_DEBUG_PRINTLN(A, ...) Serial.println(A)

#include <tdslite.h>
#include <Ethernet.h>

// #define PSTR(X) \
//   []() -> decltype(X) { \
//     static const char __[] PROGMEM = X; \
//     intermediate buf?
//     return __; \
//   }()

#define PSTR(X) X

/**
 * Initialize serial port for logging
 */
inline void initSerialPort() {
    Serial.begin(112500);
    while (!Serial)
        ;
}

/**
 * Initialize the ethernet shield with DHCP
 *
 * MUST be done before initializing tdslite.
 */
inline void initEthernetShield() {
    Serial.println("... init eth ...");
    byte mac [] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
    if (Ethernet.begin(mac) == 0) {
        Serial.println("Failed to configure Ethernet using DHCP");
        // Check for Ethernet hardware present
        if (Ethernet.hardwareStatus() == EthernetNoHardware) {
            Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
            while (true) {
                delay(1); // do nothing, no point running without Ethernet hardware
            }
        }
        if (Ethernet.linkStatus() == LinkOFF) {
            Serial.println("Ethernet cable is not connected.");
        }
        // try to congifure using IP address instead of DHCP:
        // Ethernet.begin(mac, ip, myDns);
    }
    else {
        Serial.print("  DHCP assigned IP ");
        Serial.println(Ethernet.localIP());
    }
}

/**
 * Prints INFO/ERROR messages from SQL server to stdout.
 *
 * @param [in] token INFO/ERROR token
 */
static void info_callback(void *, const tdsl::tds_info_token & token) noexcept {
    Serial.print(token.is_info() ? 'I' : 'E');
    Serial.print(PSTR(": ["));
    Serial.print(token.number);
    Serial.print(PSTR("/"));
    Serial.print(token.state);
    Serial.print(PSTR("/"));
    Serial.print(token.class_);
    Serial.print(PSTR(" @"));
    Serial.print(token.line_number);
    Serial.print(PSTR("] --> "));
    for (unsigned int i = 0; i < token.msgtext.size(); i++) {
        Serial.print((*reinterpret_cast<const char *>(token.msgtext.data() + i)));
    }
    Serial.print(PSTR("\n"));
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

    Serial.print(PSTR("row: "));
    for (const auto & field : row) {
        Serial.print(field.as<tdsl::uint32_t>());
        Serial.print("\t");
    }
    Serial.println("");
    Serial.flush();
}

// The network buffer
tdsl::uint8_t net_buf [4096] = {};
tdsl::driver_ethc driver{net_buf};

void initTdsliteDriver() {
    Serial.println("... init tdslite ...");
    decltype(driver)::connection_parameters params;
    // Server's hostname or IP address.
    params.server_name = PSTR("192.168.1.22");
    // SQL server port number
    params.port        = 14333;
    // Login user
    params.user_name   = PSTR("sa");
    // Login user password
    params.password    = PSTR("2022-tds-lite-test!");
    // Client name(optional)
    params.client_name = PSTR("arduino mega");
    // App name(optional)
    params.app_name    = PSTR("sketch");
    // Database name(optional)
    params.db_name     = PSTR("master");
    // TDS packet size
    // Recommendation: Half of the network buffer.
    params.packet_size = {sizeof(net_buf) / 2};
    driver.set_info_callback(nullptr, &info_callback);
    driver.connect(params);
    Serial.println("... init tdslite end...");
}

void initDatabase() {
    Serial.println("... init database ...");
    driver.execute_query("CREATE TABLE #hello_world(a int, b int);");
    Serial.println("... init database end...");
}

void setup() {
    initSerialPort();
    initEthernetShield();
    initTdsliteDriver();
    initDatabase();
    Serial.println("... setup complete ...");
}

int i = 0;

void loop() {
    // put your main code here, to run repeatedly:
    driver.execute_query("INSERT INTO #hello_world VALUES(1,2);");
    if (i++ % 10 == 0) {
        auto row_count = driver.execute_query("SELECT * FROM #hello_world;", nullptr, row_callback);
        Serial.print(PSTR("REPORT: rows count --> "));
        Serial.println(row_count);
    }
    Ethernet.maintain();
    delay(100);
}