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

project(tdslite-project VERSION 0.0.1 LANGUAGES CXX)

# Include hadouken
include(.hadouken/hadouken.cmake)

# Print VCS status to stdout
hdk_git_print_status()

# Project-specific configurations
include(.cmake/project-configuration.cmake)

# Components of the project
add_subdirectory(tdslite)
add_subdirectory(tests)
