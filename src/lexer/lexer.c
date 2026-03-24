#include "lexer.h"

#include <string.h>

static inline int is_space(unsigned char c) {
    return (c == ' ') | (c == '\t') | (c == '\n') | (c == '\r');
}

static inline int is_digit(unsigned char c) {
    return (c >= (unsigned char)'0') & (c <= (unsigned char)'9');
}

static inline int is_alpha(unsigned char c) {
    const unsigned char lower = (unsigned char)(c | 32u);
    return ((lower >= (unsigned char)'a') & (lower <= (unsigned char)'z')) | (c == (unsigned char)'_');
}

static inline int is_ident_continue(unsigned char c) {
    return is_alpha(c) | is_digit(c);
}

static inline Token make_token(const char *start, size_t length, TokenType type) {
    Token token;
    token.start = start;
    token.length = length;
    token.type = type;
    return token;
}

static inline TokenType classify_identifier(const char *start, size_t length) {
    if (length == 5u && memcmp(start, "class", 5u) == 0) {
        return TOKEN_CLASS;
    }
    if (length == 2u && memcmp(start, "fn", 2u) == 0) {
        return TOKEN_FN;
    }
    if (length == 5u && memcmp(start, "alloc", 5u) == 0) {
        return TOKEN_ALLOC;
    }
    if (length == 4u && memcmp(start, "free", 4u) == 0) {
        return TOKEN_FREE;
    }
    if (length == 2u && memcmp(start, "if", 2u) == 0) {
        return TOKEN_IF;
    }
    if (length == 5u && memcmp(start, "while", 5u) == 0) {
        return TOKEN_WHILE;
    }
    if (length == 6u && memcmp(start, "return", 6u) == 0) {
        return TOKEN_RETURN;
    }
    if (length == 2u && memcmp(start, "i8", 2u) == 0) {
        return TOKEN_I8;
    }
    if (length == 3u && memcmp(start, "i32", 3u) == 0) {
        return TOKEN_I32;
    }
    if (length == 3u && memcmp(start, "f32", 3u) == 0) {
        return TOKEN_F32;
    }
    if (length == 3u && memcmp(start, "f64", 3u) == 0) {
        return TOKEN_F64;
    }
    if (length == 4u && memcmp(start, "bool", 4u) == 0) {
        return TOKEN_BOOL;
    }
    return TOKEN_IDENTIFIER;
}

Token lexer_next_token(const char **current_char) {
    const char *p = *current_char;

    while (is_space((unsigned char)*p)) {
        ++p;
    }

    if (*p == '\0') {
        *current_char = p;
        return make_token(p, 0u, TOKEN_EOF);
    }

    switch (*p) {
        case ':':
            *current_char = p + 1;
            return make_token(p, 1u, TOKEN_COLON);
        case ';':
            *current_char = p + 1;
            return make_token(p, 1u, TOKEN_SEMICOLON);
        case '{':
            *current_char = p + 1;
            return make_token(p, 1u, TOKEN_LBRACE);
        case '}':
            *current_char = p + 1;
            return make_token(p, 1u, TOKEN_RBRACE);
        case '(':
            *current_char = p + 1;
            return make_token(p, 1u, TOKEN_LPAREN);
        case ')':
            *current_char = p + 1;
            return make_token(p, 1u, TOKEN_RPAREN);
        case '=':
            *current_char = p + 1;
            return make_token(p, 1u, TOKEN_ASSIGN);
        case '.':
            *current_char = p + 1;
            return make_token(p, 1u, TOKEN_DOT);
        case '>':
            *current_char = p + 1;
            return make_token(p, 1u, TOKEN_GT);
        default:
            break;
    }

    if (is_digit((unsigned char)*p)) {
        const char *start = p;
        while (is_digit((unsigned char)*p)) {
            ++p;
        }

        if (*p == '.') {
            ++p;
            while (is_digit((unsigned char)*p)) {
                ++p;
            }
        }

        *current_char = p;
        return make_token(start, (size_t)(p - start), TOKEN_NUMBER);
    }

    if (is_alpha((unsigned char)*p)) {
        const char *start = p;
        while (is_ident_continue((unsigned char)*p)) {
            ++p;
        }

        *current_char = p;
        return make_token(start, (size_t)(p - start), classify_identifier(start, (size_t)(p - start)));
    }

    *current_char = p + 1;
    return make_token(p, 1u, TOKEN_ERROR);
}
