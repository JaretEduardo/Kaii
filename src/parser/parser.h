#ifndef KAII_PARSER_PARSER_H
#define KAII_PARSER_PARSER_H

#include "ast.h"

typedef struct Parser {
    const char *current_source_pointer;
    Token current_token;
    Token peek_token;
} Parser;

void parser_init(Parser *parser, const char *source);
AstNode *parse_program(Parser *parser);

#endif /* KAII_PARSER_PARSER_H */
