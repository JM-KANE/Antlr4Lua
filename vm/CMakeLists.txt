aux_source_directory(. VM_SRCS)
aux_source_directory(stdlib LIB_SRC)

add_library(lua_vm SHARED ${VM_SRCS} ${LIB_SRC})

set_target_properties(lua_vm PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_compile_definitions(lua_vm PRIVATE $<$<PLATFORM_ID:Windows>:_CRT_SECURE_NO_WARNINGS>)

target_link_libraries(lua_vm code_gen)
