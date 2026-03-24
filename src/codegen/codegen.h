#ifndef KAII_CODEGEN_CODEGEN_H
#define KAII_CODEGEN_CODEGEN_H

#include "../parser/ast.h"
#include "../semantics/symbol_table.h"

int generate_code(AstNode *ast, SymbolTable *symbols, const char *output_filename);

#endif /* KAII_CODEGEN_CODEGEN_H */
