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

    ALL_NO_AUTO_COMPILATION_UNIT
    ALL_WITH_COVERAGE
    ALL_COVERAGE_TARGETS tdslite
    ALL_COVERAGE_REPORT_OUTPUT_DIRECTORY "${ACU_PPC_BUILD_DIRECTORY}/cc_reports" 
    ALL_LINK PRIVATE tdslite
)
