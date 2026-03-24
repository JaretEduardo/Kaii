#include "symbol_table.h"

#include <stdlib.h>
#include <string.h>

static int token_lexeme_equals(Token a, Token b) {
    if (a.start == NULL || b.start == NULL) {
        return 0;
    }
    if (a.length != b.length) {
        return 0;
    }
    if (a.length == 0u) {
        return 1;
    }
    return memcmp(a.start, b.start, a.length) == 0;
}

void symbol_table_init(SymbolTable *table) {
    if (table == NULL) {
        return;
    }

    table->symbols = NULL;
    table->count = 0u;
    table->capacity = 0u;
}

Symbol *symbol_table_lookup(SymbolTable *table, Token name) {
    size_t i;

    if (table == NULL) {
        return NULL;
    }

    for (i = 0u; i < table->count; ++i) {
        Symbol *symbol = table->symbols[i];
        if (symbol != NULL && token_lexeme_equals(symbol->name, name)) {
            return symbol;
        }
    }

    return NULL;
}

int symbol_table_insert(SymbolTable *table, Token name, Token type, SymbolCategory category) {
    Symbol **new_symbols = NULL;
    Symbol *new_symbol = NULL;
    size_t new_capacity = 0u;

    if (table == NULL) {
        return 0;
    }

    /* Prevent duplicate declarations in the same symbol table scope. */
    if (symbol_table_lookup(table, name) != NULL) {
        return 0;
    }

    if (table->count == table->capacity) {
        if (table->capacity == 0u) {
            new_capacity = 8u;
        } else {
            new_capacity = table->capacity * 2u;
            if (new_capacity < table->capacity) {
                return 0;
            }
        }

        new_symbols = (Symbol **)realloc(table->symbols, new_capacity * sizeof(Symbol *));
        if (new_symbols == NULL) {
            return 0;
        }

        table->symbols = new_symbols;
        table->capacity = new_capacity;
    }

    new_symbol = (Symbol *)malloc(sizeof(Symbol));
    if (new_symbol == NULL) {
        return 0;
    }

    /* Zero-copy: keep token views into source; do not duplicate lexeme memory. */
    new_symbol->name = name;
    new_symbol->type = type;
    new_symbol->category = category;

    table->symbols[table->count] = new_symbol;
    table->count += 1u;
    return 1;
}

void symbol_table_free(SymbolTable *table) {
    size_t i;

    if (table == NULL) {
        return;
    }

    for (i = 0u; i < table->count; ++i) {
        free(table->symbols[i]);
        table->symbols[i] = NULL;
    }

    free(table->symbols);
    table->symbols = NULL;
    table->count = 0u;
    table->capacity = 0u;
}
