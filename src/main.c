#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer/lexer.h"
#include "utils/file_io.h"

static const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF:
            return "EOF";
        case TOKEN_ERROR:
            return "ERROR";
        case TOKEN_IDENTIFIER:
            return "IDENTIFIER";
        case TOKEN_NUMBER:
            return "NUMBER";
        case TOKEN_CLASS:
            return "CLASS";
        case TOKEN_FN:
            return "FN";
        case TOKEN_ALLOC:
            return "ALLOC";
        case TOKEN_FREE:
            return "FREE";
        case TOKEN_IF:
            return "IF";
        case TOKEN_WHILE:
            return "WHILE";
        case TOKEN_RETURN:
            return "RETURN";
        case TOKEN_I8:
            return "I8";
        case TOKEN_I32:
            return "I32";
        case TOKEN_F32:
            return "F32";
        case TOKEN_F64:
            return "F64";
        case TOKEN_BOOL:
            return "BOOL";
        case TOKEN_COLON:
            return "COLON";
        case TOKEN_SEMICOLON:
            return "SEMICOLON";
        case TOKEN_LBRACE:
            return "LBRACE";
        case TOKEN_RBRACE:
            return "RBRACE";
        case TOKEN_LPAREN:
            return "LPAREN";
        case TOKEN_RPAREN:
            return "RPAREN";
        case TOKEN_ASSIGN:
            return "ASSIGN";
        case TOKEN_DOT:
            return "DOT";
        case TOKEN_GT:
            return "GT";
        default:
            return "UNKNOWN";
    }
}

int main(int argc, char *argv[]) {
    const char *source_path = NULL;
    char *source = NULL;
    size_t source_size = 0u;
    const char *cursor = NULL;
    Token token;

    if (argc != 3 || strcmp(argv[1], "build") != 0) {
        fprintf(stderr, "Kaii Compiler (Stage 1)\n");
        fprintf(stderr, "Usage: %s build <file.kaii>\n", argv[0]);
        return EXIT_FAILURE;
    }

    source_path = argv[2];
    source = read_file_to_string(source_path, &source_size);
    if (source == NULL) {
        return EXIT_FAILURE;
    }

    printf("Loaded %s (%zu bytes)\n", source_path, source_size);
    printf("%-14s | %-6s | %s\n", "TOKEN", "LEN", "LEXEME");
    printf("-----------------------------------------------\n");

    cursor = source;
    do {
        token = lexer_next_token(&cursor);
        printf("%-14s | %-6zu | %.*s\n",
               token_type_to_string(token.type),
               token.length,
               (int)token.length,
               token.start);
    } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);

    free(source);
    return (token.type == TOKEN_ERROR) ? EXIT_FAILURE : EXIT_SUCCESS;
}