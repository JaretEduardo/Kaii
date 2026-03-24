#include "codegen.h"

#include <stdio.h>
#include <string.h>

static SymbolTable *g_codegen_symbols = NULL;

static int write_literal(FILE *out, const char *text) {
    return fputs(text, out) >= 0;
}

static int write_token(FILE *out, Token token) {
    if (token.start == NULL || token.length == 0u) {
        return 1;
    }
    return fwrite(token.start, 1u, token.length, out) == token.length;
}

static int token_equals(Token token, const char *text) {
    const size_t text_len = strlen(text);
    if (token.start == NULL) {
        return 0;
    }
    if (token.length != text_len) {
        return 0;
    }
    return memcmp(token.start, text, text_len) == 0;
}

static int write_type(FILE *out, Token type_token) {
    if (type_token.length == 0u || type_token.start == NULL) {
        return write_literal(out, "void");
    }

    if (type_token.type == TOKEN_I8 || token_equals(type_token, "i8")) {
        return write_literal(out, "int8_t");
    }
    if (type_token.type == TOKEN_I32 || token_equals(type_token, "i32")) {
        return write_literal(out, "int32_t");
    }
    if (type_token.type == TOKEN_F32 || token_equals(type_token, "f32")) {
        return write_literal(out, "float");
    }
    if (type_token.type == TOKEN_F64 || token_equals(type_token, "f64")) {
        return write_literal(out, "double");
    }
    if (type_token.type == TOKEN_BOOL || token_equals(type_token, "bool")) {
        return write_literal(out, "int");
    }

    return write_token(out, type_token);
}

static int emit_indent(FILE *out, int depth) {
    int i;
    for (i = 0; i < depth; ++i) {
        if (!write_literal(out, "    ")) {
            return 0;
        }
    }
    return 1;
}

static int emit_alloc_expr(FILE *out, AstNode *node) {
    Token type_name;

    if (node == NULL || node->type != AST_ALLOC_EXPR) {
        return 0;
    }

    type_name = node->alloc_expr.type_name;
    if (!write_literal(out, "(") ||
        !write_token(out, type_name) ||
        !write_literal(out, "*)malloc(sizeof(") ||
        !write_token(out, type_name) ||
        !write_literal(out, "))")) {
        return 0;
    }

    return 1;
}

static int emit_escaped_c_string(FILE *out, Token token) {
    size_t i;

    if (token.start == NULL) {
        return 0;
    }

    for (i = 0u; i < token.length; ++i) {
        const unsigned char c = (unsigned char)token.start[i];

        if (c == '\\') {
            if (!write_literal(out, "\\\\")) {
                return 0;
            }
        } else if (c == '"') {
            if (!write_literal(out, "\\\"")) {
                return 0;
            }
        } else if (c == '\n') {
            if (!write_literal(out, "\\n")) {
                return 0;
            }
        } else if (c == '\r') {
            if (!write_literal(out, "\\r")) {
                return 0;
            }
        } else if (c == '\t') {
            if (!write_literal(out, "\\t")) {
                return 0;
            }
        } else {
            if (fputc((int)c, out) == EOF) {
                return 0;
            }
        }
    }

    return 1;
}

/* emit_expression: Recursively emit an expression node to C code.
 * Handles AST_LITERAL, AST_VARIABLE_EXPR, AST_ALLOC_EXPR, and AST_BINARY_EXPR.
 * Kaii and C share operator precedence, so binary order can be emitted directly. */
static int emit_expression(FILE *out, AstNode *expr) {
    if (expr == NULL) {
        return 0;
    }

    if (expr->type == AST_LITERAL) {
        return write_token(out, expr->literal.value);
    }

    if (expr->type == AST_VARIABLE_EXPR) {
        return write_token(out, expr->variable_expr.name);
    }

    if (expr->type == AST_ALLOC_EXPR) {
        return emit_alloc_expr(out, expr);
    }

    if (expr->type == AST_BINARY_EXPR) {
        Token op = expr->binary_expr.op;
        const char *op_str = NULL;

        /* Map token type to operator string. */
        if (op.type == TOKEN_PLUS) {
            op_str = " + ";
        } else if (op.type == TOKEN_MINUS) {
            op_str = " - ";
        } else if (op.type == TOKEN_STAR) {
            op_str = " * ";
        } else if (op.type == TOKEN_SLASH) {
            op_str = " / ";
        } else if (op.type == TOKEN_EQUALS) {
            op_str = " == ";
        } else if (op.type == TOKEN_GT) {
            op_str = " > ";
        } else {
            return 0;  /* Unknown operator */
        }

        return emit_expression(out, expr->binary_expr.left) &&
               write_literal(out, op_str) &&
               emit_expression(out, expr->binary_expr.right);
    }

    return 0;
}

static int emit_statement(FILE *out, AstNode *stmt, int indent_depth) {
    if (stmt == NULL) {
        return 0;
    }

    if (!emit_indent(out, indent_depth)) {
        return 0;
    }

    if (stmt->type == AST_ASSIGNMENT) {
        AstNode *target = stmt->assignment.target;
        AstNode *value = stmt->assignment.value;
        Token target_name;
        Token declared_type;
        Symbol *symbol = NULL;
        const char *c_type_str = NULL;

        if (target == NULL || target->type != AST_VAR_DECL || value == NULL) {
            fprintf(stderr, "emit_statement: invalid assignment structure\n");
            return 0;
        }

        target_name = target->var_decl.name;

        /* MANDATORY LOOKUP: Variable MUST exist in the symbol table.
         * No defaults - if not found, it's a semantic error. */
        if (g_codegen_symbols == NULL) {
            fprintf(stderr, "emit_statement: symbol table is NULL\n");
            return 0;
        }

        symbol = symbol_table_lookup(g_codegen_symbols, target_name);
        if (symbol == NULL) {
            fprintf(stderr, "Codegen Error: Variable '%.*s' not found in symbol table. Mandatory explicit typing enforced.\n",
                    (int)target_name.length, target_name.start);
            return 0;
        }

        declared_type = symbol->type;

        /* Map Kaii type to C type string. */
        if (declared_type.type == TOKEN_I8 || token_equals(declared_type, "i8")) {
            c_type_str = "int8_t";
        } else if (declared_type.type == TOKEN_I32 || token_equals(declared_type, "i32")) {
            c_type_str = "int32_t";
        } else if (declared_type.type == TOKEN_F32 || token_equals(declared_type, "f32")) {
            c_type_str = "float";
        } else if (declared_type.type == TOKEN_F64 || token_equals(declared_type, "f64")) {
            c_type_str = "double";
        } else if (declared_type.type == TOKEN_BOOL || token_equals(declared_type, "bool")) {
            c_type_str = "int";
        } else if (declared_type.type == TOKEN_IDENTIFIER) {
            /* Check if it's a class (pointer type). */
            Symbol *class_symbol = symbol_table_lookup(g_codegen_symbols, declared_type);
            if (class_symbol != NULL && class_symbol->category == SYMBOL_CLASS) {
                /* Emit as pointer type. */
                if (!write_token(out, declared_type) ||
                    !write_literal(out, "* ") ||
                    !write_token(out, target_name) ||
                    !write_literal(out, " = ") ||
                    !emit_expression(out, value) ||
                    !write_literal(out, ";\n")) {
                    return 0;
                }
                return 1;
            } else {
                fprintf(stderr, "emit_statement: unknown type '%.*s' for variable '%.*s'\n",
                        (int)declared_type.length, declared_type.start,
                        (int)target_name.length, target_name.start);
                return 0;
            }
        } else {
            fprintf(stderr, "emit_statement: unsupported type token for variable '%.*s'\n",
                    (int)target_name.length, target_name.start);
            return 0;
        }

        /* Emit the assignment with the mandatory type. */
        if (!write_literal(out, c_type_str) ||
            !write_literal(out, " ") ||
            !write_token(out, target_name) ||
            !write_literal(out, " = ") ||
            !emit_expression(out, value) ||
            !write_literal(out, ";\n")) {
            return 0;
        }

        return 1;
    }

    if (stmt->type == AST_FREE_STMT) {
        return write_literal(out, "free(") &&
               write_token(out, stmt->free_stmt.target_name) &&
               write_literal(out, ");\n");
    }

    if (stmt->type == AST_IF_STMT) {
        size_t i;

        if (!write_literal(out, "if (") ||
            !emit_expression(out, stmt->if_stmt.condition) ||
            !write_literal(out, ") {\n")) {
            return 0;
        }

        for (i = 0u; i < stmt->if_stmt.then_statement_count; ++i) {
            if (!emit_statement(out, stmt->if_stmt.then_statements[i], indent_depth + 1)) {
                return 0;
            }
        }

        if (!emit_indent(out, indent_depth)) {
            return 0;
        }

        if (stmt->if_stmt.else_statement_count > 0u) {
            if (!write_literal(out, "} else {\n")) {
                return 0;
            }

            for (i = 0u; i < stmt->if_stmt.else_statement_count; ++i) {
                if (!emit_statement(out, stmt->if_stmt.else_statements[i], indent_depth + 1)) {
                    return 0;
                }
            }

            if (!emit_indent(out, indent_depth) ||
                !write_literal(out, "}\n")) {
                return 0;
            }
        } else {
            if (!write_literal(out, "}\n")) {
                return 0;
            }
        }

        return 1;
    }

    if (stmt->type == AST_PRINT_STMT) {
        AstNode *expr = stmt->print_stmt.expression;
        Token expr_token;

        if (expr == NULL || expr->type != AST_VAR_DECL) {
            return 0;
        }

        expr_token = expr->var_decl.name;

        if (expr_token.type == TOKEN_STRING) {
            return write_literal(out, "printf(\"") &&
                   emit_escaped_c_string(out, expr_token) &&
                   write_literal(out, "\\n\");\n");
        }

        if (expr_token.type == TOKEN_IDENTIFIER) {
            Symbol *symbol = NULL;
            Token symbol_type;

            symbol_type.start = NULL;
            symbol_type.length = 0u;
            symbol_type.type = TOKEN_EOF;

            if (g_codegen_symbols != NULL) {
                symbol = symbol_table_lookup(g_codegen_symbols, expr_token);
                if (symbol != NULL) {
                    symbol_type = symbol->type;
                }
            }

            if (symbol_type.type == TOKEN_F32 || symbol_type.type == TOKEN_F64 ||
                token_equals(symbol_type, "f32") || token_equals(symbol_type, "f64")) {
                return write_literal(out, "printf(\"%f\\n\", ") &&
                       write_token(out, expr_token) &&
                       write_literal(out, ");\n");
            }

            if (symbol_type.type == TOKEN_IDENTIFIER && g_codegen_symbols != NULL) {
                Symbol *class_symbol = symbol_table_lookup(g_codegen_symbols, symbol_type);
                if (class_symbol != NULL && class_symbol->category == SYMBOL_CLASS) {
                    return write_literal(out, "printf(\"Object <") &&
                           write_token(out, symbol_type) &&
                           write_literal(out, "> at %p\\n\", (void*)") &&
                           write_token(out, expr_token) &&
                           write_literal(out, ");\n");
                }
            }

            /* Default integer printing path for i8/i32 and unresolved identifiers. */
            return write_literal(out, "printf(\"%d\\n\", ") &&
                   write_token(out, expr_token) &&
                   write_literal(out, ");\n");
        }

        return 0;
    }

    return 0;
}

static int emit_var_decl_field(FILE *out, AstNode *node, int indent_depth) {
    if (node == NULL || node->type != AST_VAR_DECL) {
        return 0;
    }

    return emit_indent(out, indent_depth) &&
           write_type(out, node->var_decl.declared_type) &&
           write_literal(out, " ") &&
           write_token(out, node->var_decl.name) &&
           write_literal(out, ";\n");
}

static int emit_class_decl(FILE *out, AstNode *node) {
    size_t i;
    AstClassDecl class_decl;

    if (node == NULL || node->type != AST_CLASS_DECL) {
        return 0;
    }

    class_decl = node->class_decl;

    if (!write_literal(out, "typedef struct {\n")) {
        return 0;
    }

    for (i = 0u; i < class_decl.member_count; ++i) {
        if (!emit_var_decl_field(out, class_decl.members[i], 1)) {
            return 0;
        }
    }

    if (!write_literal(out, "} ") ||
        !write_token(out, class_decl.name) ||
        !write_literal(out, ";\n\n")) {
        return 0;
    }

    return 1;
}

static int emit_parameters(FILE *out, AstFuncDecl func_decl) {
    size_t i;

    if (func_decl.parameter_count == 0u) {
        return write_literal(out, "void");
    }

    for (i = 0u; i < func_decl.parameter_count; ++i) {
        AstNode *param = func_decl.parameters[i];

        if (param == NULL || param->type != AST_VAR_DECL) {
            return 0;
        }

        if (i != 0u && !write_literal(out, ", ")) {
            return 0;
        }

        if (!write_type(out, param->var_decl.declared_type) ||
            !write_literal(out, " ") ||
            !write_token(out, param->var_decl.name)) {
            return 0;
        }
    }

    return 1;
}

static int emit_func_decl(FILE *out, AstNode *node) {
    size_t i;
    AstFuncDecl func_decl;

    if (node == NULL || node->type != AST_FUNC_DECL) {
        return 0;
    }

    func_decl = node->func_decl;

    if (!write_type(out, func_decl.return_type) ||
        !write_literal(out, " ") ||
        !write_token(out, func_decl.name) ||
        !write_literal(out, "(") ||
        !emit_parameters(out, func_decl) ||
        !write_literal(out, ") {\n")) {
        return 0;
    }

    for (i = 0u; i < func_decl.body_statement_count; ++i) {
        if (!emit_statement(out, func_decl.body_statements[i], 1)) {
            return 0;
        }
    }

    if (!write_literal(out, "}\n\n")) {
        return 0;
    }

    return 1;
}

int generate_code(AstNode *ast, SymbolTable *symbols, const char *output_filename) {
    FILE *out = NULL;
    size_t i;

    g_codegen_symbols = symbols;

    if (ast == NULL || output_filename == NULL) {
        fprintf(stderr, "generate_code: invalid input\n");
        return 1;
    }

    if (ast->type != AST_PROGRAM) {
        fprintf(stderr, "generate_code: expected AST_PROGRAM root\n");
        return 1;
    }

    out = fopen(output_filename, "wb");
    if (out == NULL) {
        fprintf(stderr, "generate_code: failed to open output file '%s'\n", output_filename);
        return 1;
    }

    if (!write_literal(out, "#include <stdint.h>\n") ||
        !write_literal(out, "#include <stdlib.h>\n") ||
        !write_literal(out, "#include <stdio.h>\n\n")) {
        fprintf(stderr, "generate_code: failed writing file header\n");
        fclose(out);
        return 1;
    }

    for (i = 0u; i < ast->program.declaration_count; ++i) {
        AstNode *decl = ast->program.declarations[i];

        if (decl == NULL) {
            fprintf(stderr, "generate_code: null declaration at index %zu\n", i);
            fclose(out);
            return 1;
        }

        if (decl->type == AST_CLASS_DECL) {
            if (!emit_class_decl(out, decl)) {
                fprintf(stderr, "generate_code: failed emitting class declaration at index %zu\n", i);
                fclose(out);
                return 1;
            }
            continue;
        }

        if (decl->type == AST_FUNC_DECL) {
            if (!emit_func_decl(out, decl)) {
                fprintf(stderr, "generate_code: failed emitting function declaration at index %zu\n", i);
                fclose(out);
                return 1;
            }
            continue;
        }

        fprintf(stderr, "generate_code: unsupported top-level node type %d at index %zu\n", (int)decl->type, i);
        fclose(out);
        return 1;
    }

    if (fclose(out) != 0) {
        fprintf(stderr, "generate_code: failed to close output file '%s'\n", output_filename);
        return 1;
    }

    return 0;
}
