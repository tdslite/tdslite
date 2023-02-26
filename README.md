# tdslite

***The library is in the initial development stage.***

Lightweight TDS implementation is an MS-TDS implementation written in C++11, suitable for working in embedded environments.

## Motivation

tdslite is the successor of the **arduino-mssql** project. The project aims to have a lightweight, production-ready implementation that can work in embedded environments.

## Dependencies

tdslite is a standalone library with no external dependencies, and it does not rely on C++ standard library to function. Instead, needed functionality from C++ standard library is implemented from scratch as required (e.g., type_traits, span). Therefore, the only requirement to use this library is **to have a decent, C++11 compliant compiler**.

## Roadmap

- Add support for send buffer fragmentation depending on negotiated packet size [DONE]
- Add different SQL server versions to integration tests besides MSSQL 2017
- Add CI pipeline with matrix build (different platforms, toolchains and  versions) [DONE]
- Add as many board simulators as possible & run tests on them (if possible) [DONE]
- Write use cases and example applications [IN PROGRESS]
- Write example code for target platforms (arduino, espressif, raspberry etc.) [IN PROGRESS]
- Implement networking code for popular network hardware (Ethernet, WiFi) [DONE]
- Network implementation MUST read negotiated packet size and send packets accordingly -- [DONE]
- Use platform-specific size_t & ssize_t instead of using int64_t's for space [DONE]
- Implement an arduino program memory string view type [DONE]
- Beautify README.md
- Publish to GitHub Releases, Arduino Libraries and PlatformIO Libraries 

## Notes

- In order to serial port passthrough to work in vscode dev container, you will need to add your user to the dialout group, e.g. `sudo usermod -a -G dialout <your-user-name>`. Requires re-login to take effect.
- Helpful commands to diagnose global memory:
  - `size /workspace/build/arduino-build/arduino.cpp.elf`
  - `readelf --demangle -a --wide /workspace/build/arduino-build/arduino.cpp.elf`
- If you are not seeing your device under /dev/tty* after plugging:
  - Remove `brltty` package: `sudo apt purge brltty`
- PlatformIO CI try to compile for all Arduino boards:

  ```bash
     bash tdslite/platform/arduino/prep-lib.sh
     pio lib --global install arduino-libraries/Ethernet
     pio lib --global install build/arduino-libpack-root/tdslite.zip
     pio boards --json-output | jq -c '.[] | select(.name|contains("Arduino")) | .id' | xargs printf -- '--board\0%s\0' | tr '\n' '\0' | xargs -0 pio ci examples/arduino/arduino.cpp
  ```

  - For all boards with ESP MCU's : `pio boards --json-output | jq -c '.[] | select(.mcu|contains("ESP")) | .id' | xargs printf -- '--board\0%s\0' | tr '\n' '\0' | xargs -0 pio ci examples/esp/esp.cpp`
