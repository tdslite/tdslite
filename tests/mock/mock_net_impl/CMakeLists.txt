# _______________________________________________________
# tdslite mock network impl. CMakeLists file
#
# @file   CMakeLists.txt
# @author Mustafa K. GILOR <mustafagilor@gmail.com>
# @date   20.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________

make_component(
    tdslite.mock.asio_net_impl
    TARGET TYPE SHARED
           LINK PRIVATE Boost::Boost tdslite
)
