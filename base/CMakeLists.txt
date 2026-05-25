set(GEN_SRC 
    ${GEN_DIR}/LuaLexer.cpp
    ${GEN_DIR}/LuaLexer.h
    ${GEN_DIR}/LuaParser.cpp
    ${GEN_DIR}/LuaParser.h
    ${GEN_DIR}/LuaParserBaseListener.cpp
    ${GEN_DIR}/LuaParserBaseListener.h
    ${GEN_DIR}/LuaParserListener.cpp
    ${GEN_DIR}/LuaParserListener.h
    ${GEN_DIR}/LuaParserBaseVisitor.cpp
    ${GEN_DIR}/LuaParserBaseVisitor.h
    ${GEN_DIR}/LuaParserVisitor.cpp
    ${GEN_DIR}/LuaParserVisitor.h
)
set(GEN_AUX
    ${GEN_DIR}/LuaLexer.interp
    ${GEN_DIR}/LuaLexer.tokens
    ${GEN_DIR}/LuaParser.interp
    ${GEN_DIR}/LuaParser.tokens
)
set(G4_FILE
    ${PROJECT_SOURCE_DIR}/LuaLexer.g4 
    ${PROJECT_SOURCE_DIR}/LuaParser.g4
)
set(LUA_BASE
    ${PROJECT_SOURCE_DIR}/base/LuaLexerBase.h
    ${PROJECT_SOURCE_DIR}/base/LuaLexerBase.cpp
    ${PROJECT_SOURCE_DIR}/base/LuaParserBase.h
    ${PROJECT_SOURCE_DIR}/base/LuaParserBase.cpp
    ${PROJECT_SOURCE_DIR}/base/LuaParserBase.cpp
    ${PROJECT_SOURCE_DIR}/base/LuaRuleContext.cpp
    ${PROJECT_SOURCE_DIR}/base/LuaRuleContext.cpp
)

set(ANTLR java -Xmx500M -cp "/usr/local/lib/antlr-4.13.2-complete.jar:$CLASSPATH" org.antlr.v4.Tool)
add_custom_command(
    OUTPUT 
        ${GEN_SRC}
        ${GEN_AUX}
    COMMAND 
        ${ANTLR} -Dlanguage=Cpp -package lua ${G4_FILE} -o ${GEN_DIR} -visitor
    DEPENDS ${G4_FILE}
    COMMENT "Generating source file"     
)

add_library(parser STATIC ${GEN_SRC} ${LUA_BASE})

target_link_libraries(parser antlr4_static)
