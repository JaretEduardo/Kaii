#include "codegen.h"

#include <stdio.h>
#include <string.h>

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
        Token alloc_type;

        if (target == NULL || target->type != AST_VAR_DECL || value == NULL) {
            return 0;
        }

        if (value->type == AST_ALLOC_EXPR) {
            alloc_type = value->alloc_expr.type_name;

            if (!write_token(out, alloc_type) ||
                !write_literal(out, "* ") ||
                !write_token(out, target->var_decl.name) ||
                !write_literal(out, " = ")) {
                return 0;
            }

            if (!emit_alloc_expr(out, value)) {
                return 0;
            }
        } else {
            return 0;
        }

        return write_literal(out, ";\n");
    }

    if (stmt->type == AST_FREE_STMT) {
        return write_literal(out, "free(") &&
               write_token(out, stmt->free_stmt.target_name) &&
               write_literal(out, ");\n");
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

int generate_code(AstNode *ast, const char *output_filename) {
    FILE *out = NULL;
    size_t i;

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
        !write_literal(out, "#include <stdlib.h>\n\n")) {
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
