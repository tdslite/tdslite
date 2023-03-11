# _______________________________________________________
# tdslite arduino network impl. CMakeLists file
#
# @file   CMakeLists.txt
# @author mkg <me@mustafagilor.com>
# @date   20.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________

make_component(
    tdslite.net
    TARGET TYPE INTERFACE
           SUFFIX .arduino
           LINK INTERFACE tdslite.net.base
)
