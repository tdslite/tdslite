# _______________________________________________________
# tdslite mock network impl. CMakeLists file
#
# @file   CMakeLists.txt
# @author mkg <me@mustafagilor.com>
# @date   20.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________

make_component(
    tdslite.net
    TARGET TYPE SHARED
           SUFFIX .asio 
           LINK PRIVATE tdslite 
                        Boost::Boost 
)
