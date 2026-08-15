
cmake_minimum_required(VERSION 3.30)
project(centijson
    VERSION "0.0.1"
    HOMEPAGE_URL "https://github.com/mity/centijson"
    LANGUAGES C
)


include(CMakeDependentOption)
cmake_dependent_option(BUILD_UTILS "Build command line utilities." ON PROJECT_IS_TOP_LEVEL OFF)
cmake_dependent_option(BUILD_TESTS "Build internal test suite."  ON PROJECT_IS_TOP_LEVEL OFF)


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
if(BUILD_TESTS)
add_subdirectory(tests)
endif()
if(BUILD_UTILS)
add_subdirectory(utils)
endif()
