aux_source_directory(. VM_SRCS)
aux_source_directory(stdlib LIB_SRC)

add_library(lua_vm STATIC ${VM_SRCS} ${LIB_SRC})

target_link_libraries(lua_vm code_gen)
