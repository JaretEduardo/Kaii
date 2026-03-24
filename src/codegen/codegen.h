#ifndef KAII_CODEGEN_CODEGEN_H
#define KAII_CODEGEN_CODEGEN_H

#include "../parser/ast.h"

int generate_code(AstNode *ast, const char *output_filename);

#endif /* KAII_CODEGEN_CODEGEN_H */
