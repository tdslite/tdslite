# _______________________________________________________
# Global CMake settings for the project 
#
# @file   clang
# @author Mustafa K. GILOR <mustafagilor@gmail.com>
# @date   12.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________

# Prevent CMake to make changes over source directory
option(CMAKE_DISABLE_SOURCE_CHANGES ON)

# Prevent in-source builds altogether
option(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# Set binary output directories explicitly
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/")
SET(CMAKE_BUILD_RPATH "$ORIGIN/../lib:$ORIGIN/")
set(CMAKE_COLOR_MAKEFILE   ON)

###########################################################################################
# Apply Compile and link options from preset
separate_arguments(TDSLITE_PROJECT_CXX_COMPILE_OPTIONS_LIST UNIX_COMMAND "${TDSLITE_PROJECT_CXX_COMPILE_OPTIONS}")
separate_arguments(TDSLITE_PROJECT_CXX_LINK_OPTIONS_LIST UNIX_COMMAND "${TDSLITE_PROJECT_CXX_LINK_OPTIONS}")
separate_arguments(TDSLITE_PROJECT_LINK_OPTIONS_LIST UNIX_COMMAND "${TDSLITE_PROJECT_LINK_OPTIONS}")
separate_arguments(TDSLITE_PROJECT_COMPILE_OPTIONS_LIST UNIX_COMMAND "${TDSLITE_PROJECT_COMPILE_OPTIONS}")

add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:${TDSLITE_PROJECT_CXX_COMPILE_OPTIONS_LIST}>")
add_link_options("$<$<LINK_LANGUAGE:CXX>:${TDSLITE_PROJECT_CXX_LINK_OPTIONS_LIST}>")
add_link_options(${TDSLITE_PROJECT_LINK_OPTIONS_LIST})
add_compile_options(${TDSLITE_PROJECT_COMPILE_OPTIONS_LIST})
###########################################################################################

# Sanitizers
if((${CMAKE_BUILD_TYPE} STREQUAL "Release") OR (${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo"))
    if(TDSLITE_PROJECT_ENABLE_SANITIZERS_ON_RELEASE)
        enable_sanitizers()
        message(STATUS "> [OPT] Sanitizers are enabled")
    endif()
elseif(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    if(TDSLITE_PROJECT_ENABLE_SANITIZERS_ON_DEBUG)
        enable_sanitizers()
        message(STATUS "> [OPT] Sanitizers are enabled")
    endif()
endif()