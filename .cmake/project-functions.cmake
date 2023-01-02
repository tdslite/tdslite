# _______________________________________________________
# CMake module containing utility functions for project
#
# @file   project-dependencies.cmake
# @author Mustafa K. GILOR <mustafagilor@gmail.com>
# @date   20.04.2022
#
# SPDX-License-Identifier:    MIT
# _______________________________________________________

# Read an environment file and declare each environment
# variable defined in it with set(...) cmake function.
function(read_environment_file ENVIRONMENT_FILE_NAME)

    file(STRINGS ${ENVIRONMENT_FILE_NAME} PROJECT_METADATAS ENCODING UTF-8)

    foreach(ENV_VAR_DECL IN LISTS PROJECT_METADATAS)
        # Trim begin and end
        string(STRIP ENV_VAR_DECL ${ENV_VAR_DECL})

        # Skip empty lines
        string(LENGTH ENV_VAR_DECL ENV_VAR_DECL_LEN)
        if(ENV_VAR_DECL_LEN EQUAL 0)
            continue()
        endif()

        # Skip comments
        string(SUBSTRING ${ENV_VAR_DECL} 0 1 ENV_VAR_DECL_FC)

        if("x${ENV_VAR_DECL_FC}" STREQUAL "x#")
            continue()
        endif()

        # Convert environment variable declaration to cmake list
        string(REPLACE "=" ";" ENV_VAR_SPLIT ${ENV_VAR_DECL})

        list(GET ENV_VAR_SPLIT 0 ENV_VAR_NAME)
        list(GET ENV_VAR_SPLIT 1 ENV_VAR_VALUE)

        # Replace quotes in environment variable values
        string(REPLACE "\"" "" ENV_VAR_VALUE ${ENV_VAR_VALUE})

        set(${ENV_VAR_NAME} ${ENV_VAR_VALUE} PARENT_SCOPE)
    endforeach()
endfunction()

function(maybe_run_conan P_BUILD_DIR P_PROFILE_FILE)
    if(CONAN_EXPORTED)
        include(${P_BUILD_DIR}/conanbuildinfo.cmake)
        conan_basic_setup(TARGETS)
    else()
        # Setup dependencies
        conan_cmake_run(
            CONANFILE .conan/conanfile.py
            PROFILE "${P_PROFILE_FILE}"
            BASIC_SETUP
            BUILD missing
            OUTPUT_QUIET
        )
    endif()
endfunction()

function(enable_sanitizers)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        # FIXME: Undefined Behaviour Sanitizer will add asap. (Current linker can't link it.)
        add_compile_options(-fsanitize=address,undefined,leak -fsanitize-recover=address -fno-omit-frame-pointer -fstack-protector -fstack-check -D_FORTIFY_SOURCE=3 -mllvm -asan-use-private-alias=1) 
        add_link_options(-static-libsan -lubsan -fsanitize=address,undefined,leak -fsanitize-recover=address -fno-omit-frame-pointer -fstack-protector -fstack-check -D_FORTIFY_SOURCE=3)
    else()
        add_compile_options(-static-libasan -static-liblsan -static-libubsan -fsanitize=address,undefined,leak -fsanitize-recover=address -fno-omit-frame-pointer -fstack-protector -fstack-check -D_FORTIFY_SOURCE=3) 
        add_link_options(-static-libasan -static-liblsan -static-libubsan -fsanitize=address,undefined,leak -fsanitize-recover=address -fno-omit-frame-pointer -fstack-protector -fstack-check -D_FORTIFY_SOURCE=3)
    endif()
    add_compile_definitions(RUNNING_ON_ASAN=1)
endfunction()