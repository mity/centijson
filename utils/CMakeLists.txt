
if(CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    if(WIN32)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mconsole -static-libgcc")
    endif()
elseif(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
    add_definitions(/D_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(json-parse cmdline.c cmdline.h json-err.c json-err.h json-parse.c)
target_include_directories(json-parse PRIVATE ../src)
target_link_libraries(json-parse json)
