#ifndef KAII_PARSER_PARSER_H
#define KAII_PARSER_PARSER_H

#include "../semantics/symbol_table.h"
#include "ast.h"

typedef struct Parser {
    const char *current_source_pointer;
    Token current_token;
    Token peek_token;
    SymbolTable *symbols;
} Parser;

void parser_init(Parser *parser, const char *source, SymbolTable *symbols);
AstNode *parse_program(Parser *parser);

#endif /* KAII_PARSER_PARSER_H */