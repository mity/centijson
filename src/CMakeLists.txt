
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

add_library(json STATIC
    json.c
    json.h
    json-dom.c
    json-dom.h
    json-ptr.c
    json-ptr.h
    value.c
    value.h
)
