cmake_minimum_required(VERSION 3.10)

project(AntlrLua LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

option(debug "" OFF)
if(debug)
    set(CMAKE_BUILD_TYPE "Debug")
    message(STATUS "Debug: ${CMAKE_CXX_FLAGS_DEBUG}")
endif()

set(GEN_DIR ${PROJECT_SOURCE_DIR}/generated)

include_directories(${GEN_DIR})
include_directories(${PROJECT_SOURCE_DIR}/base)

add_subdirectory(${PROJECT_SOURCE_DIR}/antlr4/runtime/Cpp)
add_subdirectory(${PROJECT_SOURCE_DIR}/base)
add_subdirectory(${PROJECT_SOURCE_DIR}/code_gen)
add_subdirectory(${PROJECT_SOURCE_DIR}/vm)

# file(GLOB SRC_FILE
#     ${PROJECT_SOURCE_DIR}/src/*.h
#     ${PROJECT_SOURCE_DIR}/src/*.cpp
# )

add_executable(luac main.cpp ${LUA_BASE} ${GEN_SRC})

target_link_libraries(luac parser code_gen lua_vm)