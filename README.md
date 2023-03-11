<p align="center">
  <img width="600" height="213" src="./.misc/markdown-assets/img/banner.png">
</p>

----
<p align="center">
<img src="https://img.shields.io/badge/contributions-welcome-orange.svg"/> <a href="https://opensource.org/licenses/MIT"><img src="https://img.shields.io/badge/license-MIT-blue.svg"/></a> <a href="https://github.com/mustafakemalgilor/tdslite/issues"><img src="https://img.shields.io/github/issues/mustafakemalgilor/tdslite.svg"/></a> <img src="https://img.shields.io/github/stars/mustafakemalgilor/tdslite"/> <img src="https://img.shields.io/github/downloads/mustafakemalgilor/tdslite/total"/> <img src="https://img.shields.io/github/v/release/mustafakemalgilor/tdslite?include_prereleases"/>  <a href="https://github.com/mustafakemalgilor/tdslite/actions/workflows/pipeline.yml"><img src="https://github.com/mustafakemalgilor/tdslite/actions/workflows/pipeline.yml/badge.svg"></a> <img src="https://img.shields.io/github/repo-size/mustafakemalgilor/tdslite"/>

</p>
<p align="center">
    <i>Lightweight</i>, platform independent MS-TDS implementation written in pure C++11 that can work with just <strong>2kB</strong> of SRAM!
<br />
</p>

----

Built around C++'s `zero cost / only pay for what you use` mantra. The implementation is based on Microsoft's `MS-TDS: Tabular Data Stream Protocol` technical specification (revision number 33).

----

## Getting started

Take a look into examples below to get your hands dirty!

[➤ 01 - Initialize Library](https://github.com/mustafakemalgilor/tdslite/tree/main/examples/arduino/01-initialize-library)

Example code illustrating how to initialize the library.

[➤ 02 - CRUD with tdslite](https://github.com/mustafakemalgilor/tdslite/tree/main/examples/arduino/02-create-insert-select-delete)

The four basic operations: Create, read, update, delete.

[➤ 03 - Read rows from table](https://github.com/mustafakemalgilor/tdslite/tree/main/examples/arduino/03-select-rows)

Retrieve a result set with a query (a.k.a `SELECT`ing rows)

[➤ 04 - Library callback functions](https://github.com/mustafakemalgilor/tdslite/tree/main/examples/arduino/04-callbacks)

How to use callback functions to read info/error messages sent by the server.

[➤ 05 - Execute query with parameters](https://github.com/mustafakemalgilor/tdslite/tree/main/examples/arduino/05-query-with-parameters)

Learn how to use and bind parameters in queries with tdslite.

[➤ 06 - Custom memory allocator functions](https://github.com/mustafakemalgilor/tdslite/tree/main/examples/arduino/06-custom-malloc)

Learn how to set user-defined malloc/free functions for tdslite's memory allocation.

[➤ 07 - Fiddle with minimal-sql-shell](https://github.com/mustafakemalgilor/tdslite/tree/main/examples/minimal-sql-shell)

It's a playground to try sql commands. Compile the project, and just run `./build/bin/tdslite.examples.minimal` executable.

----

## Key features

- Header-only
- Pure C++11
- Intuitive API
- *Zero* external dependencies
- Permissive open-source license (MIT)
- Suitable for embedded development
- Low memory footprint
- Decoupled networking design that can be easily extended
- Callback based design
- Supports networking interfaces that implement Arduino's `EthernetClient`.
- Supports
  - ... query execution `driver.execute_query(...)`
  - ... queries with parameters `driver.execute_rpc(...)`
  - ... reading result sets

----

## Installing the library

Import the `tdslite-<version>.zip` into your libraries. See [releases](https://github.com/mustafakemalgilor/tdslite/releases).

### ... via the Arduino IDE

`Sketch -> Include Library -> Add .ZIP Library...` and then select the downloaded `tdslite-<version>.zip` file.

## The goal

The main design goal of the project is to enable a wide variety of constrained devices (e.g. embedded, IoT) to talk to a TDS server (e.g. Microsoft SQL Server). The aim is not to implement the TDS standard down to every single letter. Most of the use cases do not require many legacy features that the TDS standard carries along, so the remaining feature set that TDS offer will be implemented on an `as-needed` basis, rather than being implemented upfront.

`tdslite` is:

- a lightweight, production-ready implementation that can work in embedded environments
- agnostic from any particular architecture, platform, environment or standard library implementation (*except for a few basic things).
- not embedded-centric, but embedded-suitable

## The design

`tdslite` is made from two main parts: the `tdslite` library, and the networking implementation. They're located at `tdslite` and `tdslite-net` folders respectively.

The main user-facing class of `tdslite` is the `tdslite::driver<T>` class, where the `T` is the networking implementation of choice. `tdslite` does not depend on any concrete networking implementation and can work with anything that satisfies the constraints defined in the [network_io_contract](tdslite-net/tdslite-net-base/include/tdslite/net/base/network_io_contract.hpp#L79) class.

The `tdslite-net` library provides two of such networking implementations; one of them is based on `Boost.ASIO` (tdslite-net-asio), and the other one being `Arduino EthernetClient` interface compatible one(tdslite-net-arduino). ASIO-based networking implementation is used for the integration tests and the [example `sql shell` application](examples/minimal-sql-shell/minimal.cpp). `tdslite-net-arduino` can work with anything that implements the `EthernetClient` interface (e.g. EthernetClient, WiFiClient):

```c++
    #include <Ethernet.h>
    #include <WiFi.h>
    #include <tdslite.h>

    tdsl::uint8_t net_buf[1024] = {};

    /* You can use the arduino_driver with EthernetClient: */
    tdsl::arduino_driver<EthernetClient> driver_e{net_buf};

    /* ... or with WiFiClient: ... */
    tdsl::arduino_driver<WiFiClient> driver_w{net_buf};

    /* ... or, even with your own, custom type that implements the EthernetClient interface: */
    struct my_client {
        int connect(const char * host, unsigned short port) ;
        bool connected();
        int available();
        void flush();
        void stop();
        tdsl::size_t write(const unsigned char * buf, tdsl::size_t len);
        int read(unsigned char * buf, unsigned long amount);
        tdsl::uint16_t localPort();
        tdsl::uint16_t remotePort();
        IPAddress localIP();
        IPAddress remoteIP();       
    };
    /* use your own custom type */
    tdsl::arduino_driver<my_client> driver_c{net_buf};
```

// TODO: Extend this section further

----

## Runtime dependencies

tdslite is a self-contained library with no external dependencies, and it does not rely on C++ standard library to function. Instead, needed functionality from C++ standard library is implemented from scratch as needed (e.g., type_traits, span). Therefore, the only requirement to use this library is **to have a decent, C++11 compliant compiler**.

----

### The tech stack

The below are the tools and libraries that used for developing this project:

- `CMake`: build system generator for the project
- `Docker`: The development environment containers
- `googletest`: unit and integration tests
- `Conan`: used as dependency manager, fetches dependencies for tests (e.g. Boost, googletest)
- `Boost`: Used for the ASIO-based `test` network implementation
- `clang-format`: Code formatting
- `pio`: Platform.IO CLI, used for HW CI tests

----

## Contributing

See [the contributing guide](CONTRIBUTING.md) for detailed instructions on how to get started with our project.

We accept different types of contributions, including some that don't require you to write a single line of code.

----

## Q&A

Q: My ethernet library is not supported. What can I do?

A: Most of the mainstram third-party ethernet libraries are compatible with EthernetClient interface. For the ethernet libraries that are not compatible with EthernetClient interface you may:

- a) roll your own wrapper class
- b) suggest third-party library author to implement a compatible interface
- c) suggest tdslite inclusion

The option `c` may be feasible if it's a widely adopted Ethernet library.

----

Built with ❤︎ by [mkg](https://www.twitter.com/mgilor) and <a href="https://github.com/mustafakemalgilor/tdslite/graphs/contributors"><img src="https://img.shields.io/github/contributors/mustafakemalgilor/tdslite?color=green&label=the%20contributors"> </a>
