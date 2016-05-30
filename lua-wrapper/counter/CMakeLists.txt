cmake_minimum_required (VERSION 3.0)
project (lcounter C)

SET(LUA_INCLUDE_DIR /usr/include/lua5.1)
message (STATUS "Lua 5.1 path: ${LUA_INCLUDE_DIR}")

include_directories (
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LUA_INCLUDE_DIR}
)

set (SOURCES
    compat.c
    counter.c
    lcounter.c
)

add_library (${PROJECT_NAME} SHARED ${SOURCES})
set_target_properties (${PROJECT_NAME} PROPERTIES PREFIX "")
