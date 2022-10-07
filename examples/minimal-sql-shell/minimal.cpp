/**
 * _________________________________________________
 * A minimal, command line SQL shell to illustrate
 * tdslite's capabilities
 *
 * @file   minimal.cpp
 * @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
 * @date   06.10.2022
 *
 * SPDX-License-Identifier:    MIT
 * _________________________________________________
 */

// #define TDSL_DEBUG_PRINT_ENABLED

#include <tdslite/net/asio/asio_network_impl.hpp> // network implementation to use
#include <tdslite/tdslite.hpp>                    // main tdslite header
#include <fort/fort.hpp>
#include <string>
#include <cstdio>
#include <iostream>

struct table_context {
    ~table_context() {
        std::cout << table.to_string() << std::endl;
    }

    fort::char_table table;
    bool header_put = {false};
};

static inline std::string u16str_as_ascii(tdsl::u16char_span span) noexcept {
    // (mgilor): It's a shame that both C and C++ standard
    // libraries lack char16_t string print support.
    std::string r;
    r.reserve(span.size_bytes() / 2);
    for (unsigned int i = 0; i < span.size(); i++) {
        r.append(1, (*reinterpret_cast<const char *>(span.data() + i)));
    }
    return r;
}

/**
 * Helper function to convert a field value to string
 *
 * @param [in] colinfo Column info corresponding to field
 * @param [in] field The field to convert
 * @return std::string Field's string representation
 */
static std::string field2str(const tdsl::tds_column_info & colinfo,
                             const tdsl::tdsl_field & field) {
    switch (colinfo.type) {
        case tdsl::data_type::NULLTYPE:
            return "<NULL>";
        case tdsl::data_type::INT1TYPE:
            return std::to_string(static_cast<int>(field.as<tdsl::int8_t>()));
        case tdsl::data_type::INTNTYPE:
            switch (colinfo.typeprops.u8l.length) {
                case 1:
                    return std::to_string(static_cast<int>(field.as<tdsl::int8_t>()));
                case 2:
                    return std::to_string(field.as<tdsl::int16_t>());
                case 4:
                    return std::to_string(field.as<tdsl::int32_t>());
                case 8:
                    return std::to_string(field.as<tdsl::int64_t>());
                default:
                    return "<invalid INTNTYPE length " +
                           std::to_string(static_cast<int>(colinfo.typeprops.u8l.length)) + ">";
            }

            break;
        case tdsl::data_type::BITTYPE:
            return field.as<tdsl::int8_t>() == 0 ? "False" : "True";
        case tdsl::data_type::BIGVARCHRTYPE:
        case tdsl::data_type::NVARCHARTYPE:
            const auto sv = field.as<tdsl::char_span>();
            return std::string(sv.begin(), sv.end());
    }
    return "<not implemented yet " + std::to_string(static_cast<int>(colinfo.type)) + ">";
}

/**
 * Prints INFO/ERROR messages from SQL server to stdout.
 *
 * @param [in] token INFO/ERROR token
 */
static tdsl::uint32_t info_callback(void *, const tdsl::tds_info_token & token) {
    const auto msgtext = u16str_as_ascii(token.msgtext);

    std::printf("%c: [%d/%d/%d @%d] --> %.*s\n", token.is_info() ? 'I' : 'E', token.number,
                token.state, token.class_, token.line_number, static_cast<int>(msgtext.size()),
                msgtext.data());

    return 0;
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
    table_context & table = *reinterpret_cast<table_context *>(u);

    if (not table.header_put) {
        table.table << fort::header;
        for (const auto & colname : colmd.column_names) {
            table.table << u16str_as_ascii(colname);
        }
        table.table << fort::endr;
        table.header_put = {true};
    }

    int fieldidx = 0;
    for (const auto & field : row) {
        table.table << field2str(colmd.columns [fieldidx++], field);
    }

    table.table << fort::endr;
}

int main(void) {
    tdsl::driver<tdsl::net::asio_network_impl> driver{};
    // Use info_callback function for printing user info
    driver.set_info_callback(/*user_ptr=*/nullptr, info_callback);

    //
    decltype(driver)::connection_parameters conn_params;
    conn_params.server_name = "mssql-2017";
    conn_params.port        = 1433;
    conn_params.user_name   = "sa";
    conn_params.password    = "2022-tds-lite-test!";
    conn_params.app_name    = "tdslite minimal example";
    conn_params.db_name     = "master";

    // Connect & login to the SQL server described in conn_params
    driver.connect(conn_params);

    // Start accepting SQL commands from the shell
    std::string line;
    while (putchar('>'), std::getline(std::cin, line)) {
        if (line == "q" || line == "exit") {
            std::printf("Bye!\n");
            std::exit(0);
        }
        // Define a table context, in case we need to display output for the command
        table_context table;
        // Create a string view of line
        tdsl::string_view query{line.data(), static_cast<tdsl::uint32_t>(line.size())};
        // Run the query
        printf("\nRows affected: %d\n", driver.execute_query(query, &table, row_callback));
    }
}