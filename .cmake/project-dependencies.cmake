# _______________________________________________________
# CMake module for initializing project dependencies
#
# @file   project-dependencies.cmake
# @author Mustafa K. GILOR <mustafagilor@gmail.com>
# @date   20.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________

# Execute conan if enabled
maybe_run_conan("${PROJECT_BINARY_DIR}" "${TDSLITE_PROJECT_CONAN_PROFILE_FILE}")

# Conan will generate Find*.cmake module for each dependency listed in conanfile
# into the build folder of the project.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${TDSLITE_BUILD_DIRECTORY}) 

# Find boost from the build folder
find_package(Boost MODULE REQUIRED QUIET)