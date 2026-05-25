aux_source_directory(. VM_SRCS)

add_library(lua_vm STATIC ${VM_SRCS})

target_link_libraries(lua_vm code_gen)
