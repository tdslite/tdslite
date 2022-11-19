#include <SPI.h>
#include <Ethernet.h>
#include <tdslite.h>
#include <Ethernet.h>

// The network buffer
tdsl::uint8_t net_buf [4096] = {};

namespace tdsl { namespace net {

    /**
     * Synchronous ASIO networking code for tdslite
     */
    struct tdsl_netimpl_ethernet : public network_impl<tdsl_netimpl_ethernet> {

        tdsl_netimpl_ethernet() {
            network_buffer = tdsl::tdsl_buffer_object{net_buf};
        }

        TDSL_SYMBOL_VISIBLE int do_connect(tdsl::char_view target, tdsl::uint16_t port) {
            client = {};

            init();
            client.stop();

            int cr      = 0;
            int retries = 3;

            // Retry up to MAX_CONNECT_ATTEMPTS times.
            while (retries--) {
                Serial.println("...trying...");
                cr = client.connect(target.data(), port);

                if (cr == 1) {
                    Serial.println("...connected ...");
                    Serial.print(client.localPort());
                    Serial.print(" --> ");
                    Serial.print(client.remoteIP());
                    Serial.print(":");
                    Serial.println(client.remotePort());
                    break;
                }

                Serial.print("...got: ");
                Serial.print(cr);
                Serial.println(" retrying...");

                delay(1000);
            }

            if (cr == 1) {
                return 0;
            }
            return cr;
        }

        inline int do_send(void) noexcept {
            auto writer = network_buffer.get_writer();
            auto in_use = writer->inuse_span();
            client.write(in_use.data(), in_use.size_bytes());
            writer->reset();
            client.flush();
        }

        /**
         * Send byte_views @p header and @p bufs sequentially to the connected endpoint
         *
         * (scatter-gather I/O)
         *
         * @returns 0 when asynchronous send is in progress
         * @returns -1 when asynchronous send is not called due to  another
         *           asynchronous send is already in progress
         */
        TDSL_SYMBOL_VISIBLE int do_send(byte_view header, byte_view message) noexcept {
            client.write(header.data(), header.size_bytes());
            client.write(message.data(), message.size_bytes());
            client.flush();
        }

        /**
         * Read exactly @p dst_buf.size() bytes from socket
         *
         * @param [in] dst_buf Destination
         */
        TDSL_SYMBOL_VISIBLE expected<tdsl::uint32_t, tdsl::int32_t> do_read(byte_span dst_buf,
                                                                            read_exactly) {

            // Serial.print("do_read ");
            // Serial.println(dst_buf.size_bytes());

            if (wait_for_bytes(dst_buf.size_bytes()) >= dst_buf.size_bytes()) {
                // Transfer `exactly` @p transfer_exactly bytes
                client.read(dst_buf.data(), dst_buf.size_bytes());
                // Serial.println("exit");
                return dst_buf.size_bytes();
            }

            // There is an error, we should handle it appropriately
            TDSL_DEBUG_PRINT(
                "tdsl_netimpl_asio::dispatch_receive(...) -> error, %d (%s) aborting and "
                "disconnecting\n",
                ec.value(), ec.what().c_str());

            // do_disconnect();
            return unexpected<tdsl::int32_t>{-1}; // error case
        }

        // IGNORE THIS
        TDSL_SYMBOL_VISIBLE void do_recv(tdsl::uint32_t transfer_exactly, read_at_least) noexcept {
            // IGNORE THIS
        }

        TDSL_SYMBOL_VISIBLE void do_recv(tdsl::uint32_t transfer_exactly, read_exactly) noexcept {
            //    Serial.println("do_recv ");
            // Serial.println(transfer_exactly);
            // Serial.flush();
            auto writer                   = network_buffer.get_writer();
            const tdsl::int64_t rem_space = writer->remaining_bytes();

            if (transfer_exactly > rem_space) {
                TDSL_DEBUG_PRINTLN("tdsl_netimpl_ethernet::do_recv(...) -> error, not enough "
                                   "space in recv buffer (%u vs %ld)",
                                   transfer_exactly, rem_space);
                TDSL_ASSERT(0);
                //  Serial.println("assert exit ");
                //  Serial.flush();
                return;
            }

            if (wait_for_bytes(transfer_exactly) >= transfer_exactly) {
                // Retrieve the free space
                auto free_space_span = writer->free_span();

                // Transfer `exactly` @p transfer_exactly bytes
                client.read(free_space_span.data(), transfer_exactly);
                // Serial.println("exit");

                // asio::read will write `read_bytes` into `free_space_span`.
                // Advance writer's offset to reflect the changes.
                writer->advance(static_cast<tdsl::int32_t>(transfer_exactly));
            }
            //  Serial.println("after recv");
            //  Serial.flush();
        }

    private:
        bool init() {
            byte mac [] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x03};
            Serial.println("Initialize Ethernet with DHCP:");

            if (Ethernet.begin(mac) == 0) {

                Serial.println("Failed to configure Ethernet using DHCP");

                if (Ethernet.hardwareStatus() == EthernetNoHardware) {

                    Serial.println(
                        "Ethernet shield was not found.  Sorry, can't run without hardware. :(");
                }
                else if (Ethernet.linkStatus() == LinkOFF) {

                    Serial.println("Ethernet cable is not connected.");
                }

                // no point in carrying on, so do nothing forevermore:

                while (true) {

                    delay(1);
                }
            }

            // print your local IP address:

            Serial.print("My IP address: ");

            Serial.println(Ethernet.localIP());
        }

        tdsl::uint16_t delay_millis = {300};
        tdsl::uint16_t wait_millis  = {3000};

        int wait_for_bytes(int bytes_need) {
            // Serial.print("wait bytes ");
            //   Serial.println(bytes_need);
            const long wait_till = millis() + delay_millis;
            int num              = 0;
            long now             = 0;

            do {
                now = millis();
                num = client.available();
                if (num < bytes_need)
                    delay(wait_millis);
                else
                    break;
            } while (now < wait_till);

            if (num == 0 && now >= wait_till)
                client.stop();
            // Serial.print("wait bytes end ");
            //  Serial.println(num);
            return num;
        }

        EthernetClient client = {};
    };
}} // namespace tdsl::net

// 192.168.1.45:14333

inline void initSerialPort() {
    Serial.begin(112500);
    while (!Serial)
        ;
}

inline void initEthernetShield() {}

/**
 * Prints INFO/ERROR messages from SQL server to stdout.
 *
 * @param [in] token INFO/ERROR token
 */
static void info_callback(void *, const tdsl::tds_info_token & token) noexcept {
    Serial.print(token.is_info() ? 'I' : 'E');
    Serial.print(": [");
    Serial.print(token.number);
    Serial.print("/");
    Serial.print(token.state);
    Serial.print("/");
    Serial.print(token.class_);
    Serial.print(" @");
    Serial.print(token.line_number);
    Serial.print("] --> ");
    for (unsigned int i = 0; i < token.msgtext.size(); i++) {
        Serial.print((*reinterpret_cast<const char *>(token.msgtext.data() + i)));
    }
    Serial.print("\n");
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

    Serial.println("ROWS:");
    for (const auto & field : row) {
        Serial.print(field.as<tdsl::uint32_t>());
        Serial.print("\t");
    }
    Serial.println("");
    Serial.flush();
}

using tdslite_driver_t = tdsl::driver<tdsl::net::tdsl_netimpl_ethernet>;
tdslite_driver_t driver{};

void setup() {
    initSerialPort();
    tdslite_driver_t::connection_parameters params = [] {
        tdslite_driver_t::connection_parameters p;
        p.server_name  = "192.168.1.45";
        p.user_name    = "sa";
        p.password     = "2022-tds-lite-test!";
        p.client_name  = "arduino mega";
        p.app_name     = "sketch";
        p.library_name = "tdslite";
        p.db_name      = "master";
        p.port         = 14333;
        return p;
    }();
    Serial.println("setup");
    driver.set_info_callback(nullptr, &info_callback);
    driver.connect(params);
    Serial.println("after connect");
    driver.execute_query(tdsl::string_view{"CREATE TABLE #hello_world(a int, b int);"});

    Serial.println("after query");
}

int i = 0;

void loop() {
    // put your main code here, to run repeatedly:
    driver.execute_query(tdsl::string_view{"INSERT INTO #hello_world VALUES(1,2);"});
    // if (i++ % 10 == 0) {
    //   driver.execute_query(tdsl::string_view{ "SELECT * FROM #hello_world;" }, nullptr,
    //   row_callback);
    // }
    Ethernet.maintain();
    delay(1000);
}