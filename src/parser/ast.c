#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

AstNode *ast_create_node(AstNodeType type) {
    AstNode *node = (AstNode *)calloc(1, sizeof(AstNode));
    if (node == NULL) {
        fprintf(stderr, "ast_create_node: out of memory allocating %zu bytes\n", sizeof(AstNode));
        return NULL;
    }

    node->type = type;
    return node;
}
