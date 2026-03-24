#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

static void parser_advance(Parser *parser) {
    parser->current_token = parser->peek_token;
    parser->peek_token = lexer_next_token(&parser->current_source_pointer);
}

static int parser_append_declaration(AstProgram *program, AstNode *declaration) {
    AstNode **new_items = NULL;
    const size_t new_count = program->declaration_count + 1u;

    if (new_count < program->declaration_count) {
        fprintf(stderr, "parser_append_declaration: declaration count overflow\n");
        return 0;
    }

    new_items = (AstNode **)realloc(program->declarations, new_count * sizeof(AstNode *));
    if (new_items == NULL) {
        fprintf(stderr, "parser_append_declaration: out of memory growing declaration list\n");
        return 0;
    }

    program->declarations = new_items;
    program->declarations[program->declaration_count] = declaration;
    program->declaration_count = new_count;
    return 1;
}

static AstNode *parse_class_decl(Parser *parser) {
    AstNode *class_node = NULL;
    size_t brace_depth = 0u;

    /* Recursive descent entry: class_decl := 'class' IDENTIFIER '{' ... '}' */
    if (parser->current_token.type != TOKEN_CLASS) {
        fprintf(stderr, "parse_class_decl: expected TOKEN_CLASS\n");
        return NULL;
    }

    class_node = ast_create_node(AST_CLASS_DECL);
    if (class_node == NULL) {
        return NULL;
    }

    parser_advance(parser);
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "parse_class_decl: expected class name after 'class'\n");
        free(class_node);
        return NULL;
    }

    class_node->class_decl.name = parser->current_token;

    parser_advance(parser);
    if (parser->current_token.type != TOKEN_LBRACE) {
        fprintf(stderr, "parse_class_decl: expected '{' after class name\n");
        free(class_node);
        return NULL;
    }

    /* Skip class body for now; retain robustness if nested braces appear. */
    brace_depth = 1u;
    while (brace_depth > 0u) {
        parser_advance(parser);

        if (parser->current_token.type == TOKEN_EOF) {
            fprintf(stderr, "parse_class_decl: unexpected EOF while scanning class body\n");
            free(class_node);
            return NULL;
        }

        if (parser->current_token.type == TOKEN_LBRACE) {
            ++brace_depth;
        } else if (parser->current_token.type == TOKEN_RBRACE) {
            --brace_depth;
        }
    }

    return class_node;
}

void parser_init(Parser *parser, const char *source) {
    if (parser == NULL) {
        return;
    }

    parser->current_source_pointer = source;

    if (source == NULL) {
        parser->current_token.start = NULL;
        parser->current_token.length = 0u;
        parser->current_token.type = TOKEN_EOF;
        parser->peek_token = parser->current_token;
        return;
    }

    parser->current_token = lexer_next_token(&parser->current_source_pointer);
    parser->peek_token = lexer_next_token(&parser->current_source_pointer);
}

AstNode *parse_program(Parser *parser) {
    AstNode *program_node = NULL;

    if (parser == NULL) {
        fprintf(stderr, "parse_program: parser is NULL\n");
        return NULL;
    }

    program_node = ast_create_node(AST_PROGRAM);
    if (program_node == NULL) {
        return NULL;
    }

    /*
     * Top-level recursive descent loop.
     * Dispatch by current token and always guarantee forward progress.
     */
    while (parser->current_token.type != TOKEN_EOF) {
        if (parser->current_token.type == TOKEN_CLASS) {
            AstNode *class_decl = parse_class_decl(parser);
            if (class_decl == NULL) {
                free(program_node->program.declarations);
                free(program_node);
                return NULL;
            }

            if (!parser_append_declaration(&program_node->program, class_decl)) {
                free(class_decl);
                free(program_node->program.declarations);
                free(program_node);
                return NULL;
            }

            /* parse_class_decl exits with current token on the closing '}'. */
            parser_advance(parser);
            continue;
        }

        /* Unknown top-level token for this phase: skip and continue scanning. */
        parser_advance(parser);
    }

    return program_node;
}
