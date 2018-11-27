
cmake_minimum_required(VERSION 2.8)
project(centijson C)


set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE $ENV{CMAKE_BUILD_TYPE})

    if("${CMAKE_BUILD_TYPE}" STREQUAL "")
        set(CMAKE_BUILD_TYPE "Release")
    endif()
endif()


if(${CMAKE_C_COMPILER_ID} MATCHES GNU|Clang)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
elseif(MSVC)
    # Disable warnings about the so-called unsecured functions:
    add_definitions(/D_CRT_SECURE_NO_WARNINGS)
endif()


set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}")
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}")

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(utils)
