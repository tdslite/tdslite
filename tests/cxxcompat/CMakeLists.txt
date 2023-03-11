# _________________________________________________
# tdslite C++ 14/17/20 compatibility tests
#
# @file   CMakeLists.txt
# @author mkg <me@mustafagilor.com>
# @date   12.04.2022
#
# SPDX-License-Identifier:    MIT
# _________________________________________________

set(ALL_TEST_SOURCES ../unit/ut_tdslite.cpp ../unit/ut_binary_reader.cpp ../unit/ut_byte_swap.cpp ../unit/ut_span.cpp ../unit/ut_tdsl_login_context.cpp ../integration/it_tdsl_login_context.cpp)

make_component(
    tdslite.tests.cxx14
    TARGET  TYPE UNIT_TEST
            SUFFIX .ut.all
            SOURCES ${ALL_TEST_SOURCES} 
    ALL_NO_AUTO_COMPILATION_UNIT
    ALL_WITH_COVERAGE
    ALL_COVERAGE_TARGETS tdslite
    ALL_COVERAGE_REPORT_OUTPUT_DIRECTORY "${TDSLITE_PROJECT_BUILD_DIRECTORY}/cc_reports" 
    ALL_LINK PRIVATE tdslite tdslite tdslite.net.asio
)

make_component(
    tdslite.tests.cxx17
    TARGET  TYPE UNIT_TEST
            SUFFIX .ut.all
            SOURCES ${ALL_TEST_SOURCES} 
    ALL_NO_AUTO_COMPILATION_UNIT
    ALL_WITH_COVERAGE
    ALL_COVERAGE_TARGETS tdslite
    ALL_COVERAGE_REPORT_OUTPUT_DIRECTORY "${TDSLITE_PROJECT_BUILD_DIRECTORY}/cc_reports" 
    ALL_LINK PRIVATE tdslite tdslite tdslite.net.asio
)

make_component(
    tdslite.tests.cxx20
    TARGET  TYPE UNIT_TEST
            SUFFIX .ut.all
            SOURCES ${ALL_TEST_SOURCES} 
    ALL_NO_AUTO_COMPILATION_UNIT
    ALL_WITH_COVERAGE
    ALL_COVERAGE_TARGETS tdslite
    ALL_COVERAGE_REPORT_OUTPUT_DIRECTORY "${TDSLITE_PROJECT_BUILD_DIRECTORY}/cc_reports" 
    ALL_LINK PRIVATE tdslite tdslite tdslite.net.asio
)

set_target_properties(
    tdslite.tests.cxx14.ut.all
    PROPERTIES CXX_STANDARD 14
               CXX_STANDARD_REQUIRED YES
               CXX_EXTENSIONS NO
)

set_target_properties(
    tdslite.tests.cxx17.ut.all
    PROPERTIES CXX_STANDARD 17
               CXX_STANDARD_REQUIRED YES
               CXX_EXTENSIONS NO
)

set_target_properties(
    tdslite.tests.cxx20.ut.all
    PROPERTIES CXX_STANDARD 20
               CXX_STANDARD_REQUIRED YES
               CXX_EXTENSIONS NO
)