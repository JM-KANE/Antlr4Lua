cmake_minimum_required(VERSION 3.10)

project(Antlr4Lua LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# set(ANTLR_PATH "/usr/local/lib/antlr-4.13.2-complete.jar" CACHE FILEPATH "path of antlr-*.jar")

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

target_link_libraries(luac parser lua_vm)