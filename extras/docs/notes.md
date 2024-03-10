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
- Beautify README.md [~DONE]
- Publish to GitHub Releases [DONE], Arduino Libraries [DONE] and PlatformIO Libraries [DONE]
- add badges for arduino [DONE], platform.io [DONE], snap and ci pipeline status [DONE]

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
     pio pkg pack -o build/tdslite.tar.gz
     pio lib --global install ./build/tdslite.tar.gz
     pio lib --global install arduino-libraries/Ethernet
     pio boards --json-output | jq -c '.[] | select(.name|contains("Arduino")) | .id' | xargs printf -- '--board\0%s\0' | tr '\n' '\0' | xargs -0 pio ci examples/arduino/arduino.cpp
  ```

  - For all boards with ESP MCU's : `pio boards --json-output | jq -c '.[] | select(.mcu|contains("ESP")) | .id' | xargs printf -- '--board\0%s\0' | tr '\n' '\0' | xargs -0 pio ci examples/esp/esp.cpp`

  - To find which macro is defined for a specific arduino board: [boards.txt](https://github.com/arduino/ArduinoCore-megaavr/blob/5e639ee40afa693354d3d056ba7fb795a8948c11/boards.txt#L25) (see build.board field of each respective board)

- To generate an library file from the project:
  - `git archive --format zip --prefix tdslite/ --output tdslite.zip main`
