aux_source_directory(. DIR_LIB_SRCS)

add_library(code_gen STATIC ${DIR_LIB_SRCS})

target_link_libraries(code_gen parser)
