#!/usr/bin/env bash

pio lib --global uninstall tdslite
pio lib --global uninstall CrashMonitor
pio lib --global uninstall MemoryFree

bash tdslite/platform/arduino/prep-lib.sh

pio lib --global install ./build/arduino-libpack-root/tdslite.zip
pio lib --global install ./vendor/MemoryFree

set -eu

# PLATFORMIO_BUILD_FLAGS=-DCI_BUILD pio ci \
# --project-option="lib_ignore=CrashMonitor" \
# --board nanorp2040connect \
# --board nanoatmega328 \
# --board uno \
# --board megaatmega1280 \
# --board megaatmega2560 \
# --board miniatmega328 \
# --board micro \
# --board yun \
# --board nanoatmega328new \
# --board pro8MHzatmega328 \
# --board ethernet \
# --board nano_every \
# --board uno_wifi_rev2 \
# tests/sketches/arduino/arduino.cpp

# Test Arduino SAMD boards 
PLATFORMIO_BUILD_FLAGS=-DCI_BUILD pio ci \
--project-option="lib_ignore=CrashMonitor" \
--board zero \
tests/sketches/arduino/arduino.cpp

# Test Arduino RPi boards

PLATFORMIO_BUILD_FLAGS=-DCI_BUILD pio ci \
--project-option="lib_ignore=CrashMonitor" \
--board nanorp2040connect \
tests/sketches/arduino/arduino.cpp

# Test Arduino STM boards

PLATFORMIO_BUILD_FLAGS=-DCI_BUILD pio ci \
--project-option="lib_ignore=CrashMonitor" \
--board portenta_h7_m4 \
--board portenta_h7_m7 \
tests/sketches/arduino/arduino.cpp

# Test Arduino nRF boards

PLATFORMIO_BUILD_FLAGS=-DCI_BUILD pio ci \
--project-option="lib_ignore=CrashMonitor" \
--board nano33ble \
--board nicla_sense_me \
tests/sketches/arduino/arduino.cpp

