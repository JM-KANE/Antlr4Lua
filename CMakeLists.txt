cmake_minimum_required(VERSION 3.20)

cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(Antlr4Lua LANGUAGES CXX)

# option(debug "" OFF)
# if(debug)
#     set(CMAKE_BUILD_TYPE "Debug")
#     message(STATUS "Debug: ${CMAKE_CXX_FLAGS_DEBUG}")
# endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(GEN_DIR ${PROJECT_SOURCE_DIR}/generated)
set(PARSER_DIR ${PROJECT_SOURCE_DIR}/parser)

add_subdirectory(${PARSER_DIR})
add_subdirectory(${PROJECT_SOURCE_DIR}/code_gen)
add_subdirectory(${PROJECT_SOURCE_DIR}/vm)


set(_dummy "${CMAKE_CURRENT_BINARY_DIR}/lua_dummy.cpp")
if(NOT EXISTS ${_dummy})
    file(WRITE ${_dummy} "")
endif()

set(LIB_OBJECT lua_vm code_gen parser)

add_library(lua_shared SHARED ${_dummy})
set_target_properties(lua_shared PROPERTIES OUTPUT_NAME "lua")
target_link_libraries(lua_shared ${LIB_OBJECT})
set_target_properties(lua_shared PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_executable(lua main.cpp)
target_link_libraries(lua ${LIB_OBJECT})

set_target_properties(lua PROPERTIES 
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

# install
set(LIB_INSTALL lua lua_shared)
install(TARGETS ${LIB_INSTALL}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/include
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.hpp")
install(DIRECTORY vm/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vm
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.hpp")
