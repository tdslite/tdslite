# _______________________________________________________
# tdslite unit tests
#
# @file   CMakeLists.txt
# @author Mustafa K. GILOR <mustafagilor@gmail.com>
# @date   12.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________

make_component(
    tdslite.tests
    TARGET  TYPE UNIT_TEST
            SUFFIX .ut.tdslite
            SOURCES ut_tdslite.cpp

    TARGET  TYPE UNIT_TEST
            SUFFIX .ut.binary_reader
            SOURCES ut_binary_reader.cpp

    TARGET  TYPE UNIT_TEST
            SUFFIX .ut.byte_swap
            SOURCES ut_byte_swap.cpp

    TARGET  TYPE UNIT_TEST
            SUFFIX .ut.span
            SOURCES ut_span.cpp

    TARGET  TYPE UNIT_TEST
            SUFFIX .ut.tds_login
            SOURCES ut_tds_login.cpp

    ALL_NO_AUTO_COMPILATION_UNIT
    ALL_WITH_COVERAGE
    ALL_COVERAGE_TARGETS tdslite
    ALL_COVERAGE_REPORT_OUTPUT_DIRECTORY "${ACU_PPC_BUILD_DIRECTORY}/cc_reports" 
    ALL_LINK PRIVATE tdslite
)

set(ALL_UT_SOURCES ut_tdslite.cpp ut_binary_reader.cpp ut_byte_swap.cpp ut_span.cpp ut_tds_login.cpp)

make_component(
    tdslite.tests.cxx14
    TARGET  TYPE UNIT_TEST
            SUFFIX .ut.all
            SOURCES ${ALL_UT_SOURCES} 
    ALL_NO_AUTO_COMPILATION_UNIT
    ALL_WITH_COVERAGE
    ALL_COVERAGE_TARGETS tdslite
    ALL_COVERAGE_REPORT_OUTPUT_DIRECTORY "${ACU_PPC_BUILD_DIRECTORY}/cc_reports" 
    ALL_LINK PRIVATE tdslite
)

make_component(
    tdslite.tests.cxx17
    TARGET  TYPE UNIT_TEST
            SUFFIX .ut.all
            SOURCES ${ALL_UT_SOURCES} 
    ALL_NO_AUTO_COMPILATION_UNIT
    ALL_WITH_COVERAGE
    ALL_COVERAGE_TARGETS tdslite
    ALL_COVERAGE_REPORT_OUTPUT_DIRECTORY "${ACU_PPC_BUILD_DIRECTORY}/cc_reports" 
    ALL_LINK PRIVATE tdslite
)

make_component(
    tdslite.tests.cxx20
    TARGET  TYPE UNIT_TEST
            SUFFIX .ut.all
            SOURCES ${ALL_UT_SOURCES} 
    ALL_NO_AUTO_COMPILATION_UNIT
    ALL_WITH_COVERAGE
    ALL_COVERAGE_TARGETS tdslite
    ALL_COVERAGE_REPORT_OUTPUT_DIRECTORY "${ACU_PPC_BUILD_DIRECTORY}/cc_reports" 
    ALL_LINK PRIVATE tdslite
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