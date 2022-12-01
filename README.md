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
- Network implementation MUST read negotiated packet size and send packets accordingly -- [DONE]
- Use platform-specific size_t & ssize_t instead of using int64_t's for space
- Implement an arduino program memory string view type

## Notes

- In order to serial port passthrough to work in vscode dev container, you will need to add your user to the dialout group, e.g. `sudo usermod -a -G dialout <your-user-name>`. Requires re-login to take effect.
- Helpful commands to diagnose global memory:
  - `size /workspace/build/arduino-build/arduino.ino.elf`
  - `readelf --demangle -a --wide /workspace/build/arduino-build/arduino.ino.elf`
- If you are not seeing your device under /dev/tty* after plugging:
  - Remove `brltty` package: `sudo apt purge brltty`
- PlatformIO CI try to compile for all Arduino boards:
  - bash .misc/arduino/prep-lib.sh
  - pio lib --global install arduino-libraries/Ethernet
  - pip lib --global install build/arduino-libpack-root/tdslite.zip
  - pio boards --json-output | jq -c '.[] | select(.name|contains("Arduino")) | .id' | xargs printf -- '--board\0%s\0' | tr '\n' '\0' | xargs -0 pio ci examples/arduino/arduino.ino
    - It'll put an output like this:

        ```bash
        Environment         Status    Duration
        ------------------  --------  ------------
        btatmega168         FAILED    00:00:01.201
        btatmega328         SUCCESS   00:00:01.208
        due                 FAILED    00:00:01.371
        dueUSB              FAILED    00:00:01.312
        diecimilaatmega168  FAILED    00:00:01.192
        diecimilaatmega328  SUCCESS   00:00:01.203
        esplora             SUCCESS   00:00:01.239
        ethernet            SUCCESS   00:00:01.184
        fio                 SUCCESS   00:00:01.186
        chiwawa             SUCCESS   00:00:01.255
        leonardo            SUCCESS   00:00:01.238
        leonardoeth         SUCCESS   00:00:01.228
        lilypadatmega168    FAILED    00:00:01.185
        lilypadatmega328    SUCCESS   00:00:01.180
        LilyPadUSB          SUCCESS   00:00:01.248
        mzeroUSB            FAILED    00:00:01.305
        mzeroproUSB         FAILED    00:00:01.343
        mzeropro            FAILED    00:00:01.321
        mkrfox1200          FAILED    00:00:01.326
        mkrgsm1400          FAILED    00:00:01.291
        mkrnb1500           FAILED    00:00:01.325
        mkrwan1300          FAILED    00:00:01.356
        mkrwan1310          FAILED    00:00:01.741
        mkrwifi1010         FAILED    00:00:01.321
        mkr1000USB          FAILED    00:00:01.337
        mkrzero             FAILED    00:00:01.391
        megaADK             SUCCESS   00:00:01.187
        megaatmega1280      SUCCESS   00:00:01.199
        megaatmega2560      SUCCESS   00:00:01.193
        micro               SUCCESS   00:00:01.273
        miniatmega168       FAILED    00:00:01.219
        miniatmega328       SUCCESS   00:00:01.207
        atmegangatmega168   FAILED    00:00:01.207
        atmegangatmega8     FAILED    00:00:01.201
        nano33ble           FAILED    00:00:04.974
        nanoatmega168       FAILED    00:00:01.230
        nanoatmega328       SUCCESS   00:00:01.232
        nanoatmega328new    SUCCESS   00:00:01.202
        nano_every          FAILED    00:00:00.642
        nanorp2040connect   FAILED    00:00:03.089
        nicla_sense_me      FAILED    00:00:04.807
        portenta_h7_m4      FAILED    00:00:03.511
        portenta_h7_m7      FAILED    00:00:03.523
        pro8MHzatmega168    FAILED    00:00:01.196
        pro16MHzatmega168   FAILED    00:00:01.181
        pro8MHzatmega328    SUCCESS   00:00:01.228
        pro16MHzatmega328   SUCCESS   00:00:01.216
        robotControl        SUCCESS   00:00:01.260
        robotMotor          SUCCESS   00:00:01.248
        tian                FAILED    00:00:01.362
        uno                 SUCCESS   00:00:01.199
        uno_wifi_rev2       FAILED    00:00:00.665
        yun                 SUCCESS   00:00:01.257
        yunmini             SUCCESS   00:00:01.248
        zero                FAILED    00:00:01.308
        zeroUSB             FAILED    00:00:01.324
        genuino101          FAILED    00:00:00.857
        ```

