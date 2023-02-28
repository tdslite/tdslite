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
- add badges for arduino, platform.io, snap and ci pipeline status

----

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
