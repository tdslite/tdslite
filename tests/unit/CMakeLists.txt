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
    tdslite.tests.ut
    TARGET  TYPE UNIT_TEST
            SUFFIX .tdslite
            SOURCES ut_tdslite.cpp

    TARGET  TYPE UNIT_TEST
            SUFFIX .binary_reader
            SOURCES ut_binary_reader.cpp

    TARGET  TYPE UNIT_TEST
            SUFFIX .byte_swap
            SOURCES ut_byte_swap.cpp

    TARGET  TYPE UNIT_TEST
            SUFFIX  .span
            SOURCES ut_span.cpp

    TARGET  TYPE UNIT_TEST
            SUFFIX .tdsl_login_context
            SOURCES ut_tdsl_login_context.cpp

    TARGET  TYPE UNIT_TEST
            SUFFIX .tdsl_command_context
            SOURCES ut_tdsl_command_context.cpp

    TARGET  TYPE UNIT_TEST
            SUFFIX .tds_allocator
            SOURCES ut_tdsl_allocator.cpp

    TARGET  TYPE UNIT_TEST
            SUFFIX .tdsl_field
            SOURCES ut_tdsl_field.cpp

    ALL_NO_AUTO_COMPILATION_UNIT
    ALL_WITH_COVERAGE
    ALL_COVERAGE_TARGETS tdslite
    ALL_COVERAGE_REPORT_OUTPUT_DIRECTORY "${TDSLITE_PROJECT_BUILD_DIRECTORY}/cc_reports" 
    ALL_LINK PRIVATE tdslite
)