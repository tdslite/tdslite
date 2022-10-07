# _______________________________________________________
# examples for tdslite library
#
# @file   CMakeLists.txt
# @author Mustafa K. GILOR <mustafagilor@gmail.com>
# @date   25.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________

add_subdirectory(thirdparty/fort)

make_component(
    tdslite.examples
    TARGET  TYPE EXECUTABLE
            SUFFIX .minimal
            SOURCES minimal.cpp
            LINK tdslite.net.asio tdslite.thirdparty.fort
    ALL_NO_AUTO_COMPILATION_UNIT
    ALL_LINK tdslite
)


set_target_properties(
    tdslite.examples.minimal
    PROPERTIES CXX_STANDARD 11
               CXX_STANDARD_REQUIRED YES
               CXX_EXTENSIONS NO
)