#ifndef KAII_LEXER_LEXER_H
#define KAII_LEXER_LEXER_H

#include <stddef.h>

typedef enum TokenType {
    TOKEN_EOF = 0,
    TOKEN_ERROR,

    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,

    TOKEN_CLASS,
    TOKEN_FN,
    TOKEN_ALLOC,
    TOKEN_FREE,
    TOKEN_IF,
    TOKEN_WHILE,
    TOKEN_RETURN,

    TOKEN_I8,
    TOKEN_I32,
    TOKEN_F32,
    TOKEN_F64,
    TOKEN_BOOL,

    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_ASSIGN,
    TOKEN_DOT,
    TOKEN_GT
} TokenType;

/* Zero-copy token: lexeme points into the original source buffer. */
typedef struct Token {
    const char *start;
    size_t length;
    TokenType type;
} Token;

/*
 * Single-pass lexer API: consumes from *current_char and advances it.
 * Returned token is always a view into the original source text.
 */
Token lexer_next_token(const char **current_char);

#endif /* KAII_LEXER_LEXER_H */
