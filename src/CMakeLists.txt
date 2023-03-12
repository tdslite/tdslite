# _______________________________________________________
# tdslite library cmakelists file
#
# @file   CMakeLists.txt
# @author mkg <me@mustafagilor.com>
# @date   12.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________

make_component(
    tdslite
    TARGET TYPE INTERFACE
    ALL_NO_AUTO_COMPILATION_UNIT
)

target_include_directories(tdslite INTERFACE ${CMAKE_CURRENT_LIST_DIR})

set_target_properties(
    tdslite
    PROPERTIES CXX_STANDARD 11
               CXX_STANDARD_REQUIRED YES
               CXX_EXTENSIONS NO
)