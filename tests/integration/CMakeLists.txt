# _______________________________________________________
# tdslite integration tests
#
# @file   CMakeLists.txt
# @author Mustafa K. GILOR <mustafagilor@gmail.com>
# @date   12.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________


make_component(
    tdslite.tests.it
    TARGET  TYPE UNIT_TEST
            SUFFIX .tdsl_login_context
            SOURCES it_tdsl_login_context.cpp

    TARGET  TYPE UNIT_TEST
            SUFFIX .tdsl_command_context
            SOURCES it_tdsl_command_context.cpp

    ALL_NO_AUTO_COMPILATION_UNIT
    ALL_WITH_COVERAGE
    ALL_COVERAGE_TARGETS tdslite
    ALL_COVERAGE_REPORT_OUTPUT_DIRECTORY "${TDSLITE_PROJECT_BUILD_DIRECTORY}/cc_reports" 
    ALL_LINK PRIVATE tdslite tdslite.net.asio
)


