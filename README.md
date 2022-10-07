# tdslite

***The library is in the initial development stage.***

Lightweight TDS implementation is an MS-TDS implementation written in C++11, suitable for working in embedded environments.

## Motivation

tdslite is the successor of the **arduino-mssql** project. The project aims to have a lightweight, production-ready implementation that can work in embedded environments.

## Dependencies

tdslite is a standalone library with no external dependencies, and it does not rely on C++ standard library to function. Instead, needed functionality from C++ standard library is implemented from scratch as required (e.g., type_traits, span). Therefore, the only requirement to use this library is **to have a decent, C++11 compliant compiler**.

## Roadmap

- Add support for send buffer fragmentation depending on negotiated packet size
- Add different SQL server versions to integration tests besides MSSQL 2017
- Add CI pipeline with matrix build (different platforms, toolchains and  versions)
- Add as many board simulators as possible & run tests on them (if possible)
- Write use cases and example applications
- Write example code for target platforms (arduino, espressif, raspberry etc.)
- Implement networking code for popular network hardware (Ethernet, WiFi) 