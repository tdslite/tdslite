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
           SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/tdsl_netimpl_asio       
)

# (mkg): I know this looks stupid but Arduino Library Index picks EVERYTHING
# listed under the repository, regardless of what's been excluded in
# .gitattributes file, and there's no option to exclude any folder either.
# We were shipping the src/tdsl_netimpl_asio file as src/tdsl_netimpl_asio.cxx
# before, because the Arduino IDE was not picking the "cxx" extension as a source
# file extension, but that seems to be changed with the version (2.3.2). 
# Sketch compilations fail with mission "asio" header error, which should not be
# a part of the compilation in the first place.
# 
# Now, we're trying to avoid this by simply removing the extension from the file
# altogether. The "hack" below allows CMake to recognize this file as a C++ file.
# Upstream issue: https://github.com/arduino/libraries-repository-engine/issues/59
set_source_files_properties(src/tdsl_netimpl_asio PROPERTIES LANGUAGE CXX)