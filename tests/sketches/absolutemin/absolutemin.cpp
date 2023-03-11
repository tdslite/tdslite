/**
 * ____________________________________________________
 * Sketch to check memory usage of tdslite
 *
 * @file   absolutemin.ino
 * @author mkg <me@mustafagilor.com>
 * @date   04.12.2022
 *
 * SPDX-License-Identifier:    MIT
 * ____________________________________________________
 */

#include <tdslite.h>

#define SKETCH_TDSL_NETBUF_SIZE 512
#define SKETCH_TDSL_PACKET_SIZE 512

// --------------------------------------------------------------------------------

struct EthernetClient {
    int connect(const char * dst, tdsl::uint16_t port) {}

    int read(tdsl::uint8_t * buf, tdsl::size_t sz) {}

    int write(const tdsl::uint8_t * buf, tdsl::size_t sz) {}

    int available() {
        return 0;
    }

    void flush() {}

    void stop() {}

    bool connected() {
        return false;
    }
};

// --------------------------------------------------------------------------------

tdsl::uint8_t net_buf [SKETCH_TDSL_NETBUF_SIZE] = {};
tdsl::arduino_driver<EthernetClient> driver{net_buf};

// --------------------------------------------------------------------------------

void setup() {
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
    // Database name(optional)
    params.db_name     = TDSL_PMEMSTR("master");
    // TDS packet size
    // Recommendation: Half of the network buffer.
    params.packet_size = {SKETCH_TDSL_PACKET_SIZE};
    driver.connect(params);
}

// --------------------------------------------------------------------------------

void loop() {
    driver.execute_query(TDSL_PMEMSTR("CREATE TABLE #hello_world(a int, b int);"));
}