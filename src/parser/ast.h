#ifndef KAII_PARSER_AST_H
#define KAII_PARSER_AST_H

#include <stddef.h>

#include "../lexer/lexer.h"

/*
 * Tagged-union AST layout:
 * - AstNode.type identifies the active union member.
 * - Each node payload is a compact struct dedicated to one syntax shape.
 * - Token pointers keep zero-copy links to lexer output (no lexeme duplication).
 */
typedef enum AstNodeType {
    AST_PROGRAM = 0,
    AST_CLASS_DECL,
    AST_FUNC_DECL,
    AST_VAR_DECL,
    AST_ASSIGNMENT,
    AST_ALLOC_EXPR,
    AST_FREE_STMT,
    AST_PRINT_STMT
} AstNodeType;

typedef struct AstNode AstNode;

/* Root compilation unit: top-level declarations. */
typedef struct AstProgram {
    AstNode **declarations;
    size_t declaration_count;
} AstProgram;

/* class Foo { ... } */
typedef struct AstClassDecl {
    Token name;
    AstNode **members;
    size_t member_count;
} AstClassDecl;

/* fn bar(...) : T { ... } */
typedef struct AstFuncDecl {
    Token name;
    AstNode **parameters;
    size_t parameter_count;
    Token return_type;
    AstNode **body_statements;
    size_t body_statement_count;
} AstFuncDecl;

/* let x: T = expr; */
typedef struct AstVarDecl {
    Token name;
    Token declared_type;
    AstNode *initializer;
} AstVarDecl;

/* target = value; */
typedef struct AstAssignment {
    AstNode *target;
    AstNode *value;
} AstAssignment;

/* alloc T(expr) */
typedef struct AstAllocExpr {
    Token type_name;
    AstNode *size_expr;
} AstAllocExpr;

/* free p; */
typedef struct AstFreeStmt {
    Token target_name;
} AstFreeStmt;

/* print(expr); */
typedef struct AstPrintStmt {
    AstNode *expression;
} AstPrintStmt;

/*
 * Main AST node container.
 * The union is anonymous by design for terse field access:
 *   node->class_decl, node->func_decl, etc.
 */
struct AstNode {
    AstNodeType type;
    union {
        AstProgram program;
        AstClassDecl class_decl;
        AstFuncDecl func_decl;
        AstVarDecl var_decl;
        AstAssignment assignment;
        AstAllocExpr alloc_expr;
        AstFreeStmt free_stmt;
        AstPrintStmt print_stmt;
    };
};

/* Allocates and initializes an AST node for the requested type. */
AstNode *ast_create_node(AstNodeType type);

#endif /* KAII_PARSER_AST_H */
