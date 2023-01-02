# _________________________________________________________
# tds-lite project top level CMakeLists.txt file
#
# @file   CMakeLists.txt
# @author Mustafa Kemal GILOR <mustafagilor@gmail.com>
# @date   12.04.2022
#
# SPDX-License-Identifier:    MIT
# _________________________________________________________

cmake_minimum_required(VERSION 3.23)

# Enabled languages for the project
enable_language(C CXX)

# Include miscallenaous functions defined by us, such as 
# `read_environment_file(...)` and `maybe_run_conan()`.
include(.cmake/project-functions.cmake)

# `project-metadata.env` file contains properties such as project name, version 
# description. These variables are shared between several entities  so they're 
# aggregated into single file for convenience. 
read_environment_file("project-metadata.env")

# Define the top-level project. This defines the version for subsequen project(...) calls and
# also the stuff like CPack package name, version etc. 
project(
    ${PROJECT_METADATA_NAME}
    VERSION ${PROJECT_METADATA_VERSION_MAJOR}.${PROJECT_METADATA_VERSION_MINOR}.${PROJECT_METADATA_VERSION_REVISION}
    LANGUAGES CXX
    DESCRIPTION ${PROJECT_METADATA_DESCRIPTION}
)

set(CONAN_SYSTEM_INCLUDES "On")

# Include hadouken
include(.hadouken/hadouken.cmake)

# Print VCS status to stdout
hdk_git_print_status()

# Project external dependencies
include(.cmake/project-dependencies.cmake)

# Project-specific configurations
include(.cmake/project-configuration.cmake)

# Components of the project
add_subdirectory(tdslite)
add_subdirectory(tdslite-net/tdslite-net-base)
add_subdirectory(tdslite-net/tdslite-net-asio)
add_subdirectory(tests/unit)
add_subdirectory(tests/integration)
add_subdirectory(tests/cxxcompat)
add_subdirectory(examples/minimal-sql-shell)
