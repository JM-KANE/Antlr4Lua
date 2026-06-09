cmake_minimum_required(VERSION 3.20)

cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(Antlr4Lua LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 20)
# set(ANTLR_PATH "/usr/local/lib/antlr-4.13.2-complete.jar" CACHE FILEPATH "path of antlr-*.jar")

# option(debug "" OFF)
# if(debug)
#     set(CMAKE_BUILD_TYPE "Debug")
#     message(STATUS "Debug: ${CMAKE_CXX_FLAGS_DEBUG}")
# endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(GEN_DIR ${PROJECT_SOURCE_DIR}/generated)
set(PARSER_DIR ${PROJECT_SOURCE_DIR}/parser)

# include_directories(${GEN_DIR})
# include_directories(${PARSER_DIR})

add_subdirectory(${PROJECT_SOURCE_DIR}/antlr4/runtime/Cpp)
add_subdirectory(${PARSER_DIR})
add_subdirectory(${PROJECT_SOURCE_DIR}/code_gen)
add_subdirectory(${PROJECT_SOURCE_DIR}/vm)


set(_dummy "${CMAKE_CURRENT_BINARY_DIR}/lua_dummy.cpp")
if(NOT EXISTS ${_dummy})
    file(WRITE ${_dummy} "")
endif()

add_library(lua_shared SHARED ${_dummy})
set_target_properties(lua_shared PROPERTIES OUTPUT_NAME "lua")
target_link_libraries(lua_shared parser lua_vm)

set_target_properties(lua_shared PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_executable(lua main.cpp)
target_link_libraries(lua lua_shared)

if(WIN32)
    add_custom_command(TARGET lua POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:lua_vm>"
            "$<TARGET_FILE_DIR:lua>"
        COMMENT "Copying lua_vm.dll to lua.exe output directory"
    )
endif()

set_target_properties(lua PROPERTIES 
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)


# install
install(TARGETS lua lua_shared lua_vm
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/include
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.hpp")
install(DIRECTORY vm/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/vm
    FILES_MATCHING 
    PATTERN "*.h"
    PATTERN "*.hpp")
