#ifndef KAII_SEMANTICS_SYMBOL_TABLE_H
#define KAII_SEMANTICS_SYMBOL_TABLE_H

#include <stddef.h>

#include "../lexer/lexer.h"

typedef enum SymbolCategory {
    SYMBOL_VARIABLE = 0,
    SYMBOL_FUNCTION,
    SYMBOL_CLASS
} SymbolCategory;

typedef struct Symbol {
    Token name;
    Token type;
    SymbolCategory category;
} Symbol;

typedef struct SymbolTable {
    Symbol **symbols;
    size_t count;
    size_t capacity;
} SymbolTable;

void symbol_table_init(SymbolTable *table);
int symbol_table_insert(SymbolTable *table, Token name, Token type, SymbolCategory category);
Symbol *symbol_table_lookup(SymbolTable *table, Token name);
void symbol_table_free(SymbolTable *table);

#endif /* KAII_SEMANTICS_SYMBOL_TABLE_H */
