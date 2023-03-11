# _______________________________________________________
# examples for tdslite library
#
# @file   CMakeLists.txt
# @author mkg <me@mustafagilor.com>
# @date   25.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________

set(FORT_ENABLE_ASTYLE OFF CACHE INTERNAL "")
set(FORT_ENABLE_WCHAR ON CACHE INTERNAL "")
set(FORT_ENABLE_UTF8 ON CACHE INTERNAL "")
set(FORT_ENABLE_TESTING OFF CACHE INTERNAL "")

include(FetchContent)

FetchContent_Declare(
  libfort SYSTEM
  GIT_REPOSITORY https://github.com/seleznevae/libfort.git
  GIT_TAG        b1c32b67511f4612996b287a1ef4a9df012521d2 # release-v0.4.2
)

FetchContent_MakeAvailable(libfort)

make_component(
    tdslite.examples
    TARGET  TYPE EXECUTABLE
            SUFFIX .minimal
            SOURCES minimal.cpp
            LINK tdslite.net.asio libfort::fort
    ALL_NO_AUTO_COMPILATION_UNIT
    ALL_LINK tdslite
)


set_target_properties(
    tdslite.examples.minimal
    PROPERTIES CXX_STANDARD 11
               CXX_STANDARD_REQUIRED YES
               CXX_EXTENSIONS NO
)