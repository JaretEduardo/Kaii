#include "parser.h"

#include <stdio.h>
#include <stdlib.h>

static void parser_advance(Parser *parser) {
    parser->current_token = parser->peek_token;
    parser->peek_token = lexer_next_token(&parser->current_source_pointer);
}

static int parser_is_comma_token(const Token *token) {
    if (token == NULL || token->start == NULL || token->length != 1u) {
        return 0;
    }
    return token->start[0] == ',';
}

static int parser_is_valid_type_token(TokenType type) {
    return type == TOKEN_I32 ||
           type == TOKEN_F32 ||
           type == TOKEN_BOOL ||
           type == TOKEN_I8 ||
           type == TOKEN_F64 ||
           type == TOKEN_IDENTIFIER;
}

static int parser_append_node_ptr(AstNode ***items, size_t *count, AstNode *node, const char *context) {
    AstNode **new_items = NULL;
    const size_t new_count = *count + 1u;

    if (new_count < *count) {
        fprintf(stderr, "%s: item count overflow\n", context);
        return 0;
    }

    new_items = (AstNode **)realloc(*items, new_count * sizeof(AstNode *));
    if (new_items == NULL) {
        fprintf(stderr, "%s: out of memory while growing node array\n", context);
        return 0;
    }

    *items = new_items;
    (*items)[*count] = node;
    *count = new_count;
    return 1;
}

static int parser_append_declaration(AstProgram *program, AstNode *declaration) {
    return parser_append_node_ptr(
        &program->declarations,
        &program->declaration_count,
        declaration,
        "parser_append_declaration");
}

static int parser_append_class_member(AstClassDecl *class_decl, AstNode *member) {
    return parser_append_node_ptr(
        &class_decl->members,
        &class_decl->member_count,
        member,
        "parser_append_class_member");
}

static int parser_append_func_parameter(AstFuncDecl *func_decl, AstNode *parameter) {
    return parser_append_node_ptr(
        &func_decl->parameters,
        &func_decl->parameter_count,
        parameter,
        "parser_append_func_parameter");
}

static AstNode *parse_var_decl(Parser *parser) {
    AstNode *var_node = NULL;
    Token name;
    Token declared_type;

    if (parser == NULL) {
        fprintf(stderr, "parse_var_decl: parser is NULL\n");
        return NULL;
    }

    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "parse_var_decl: expected property name (IDENTIFIER)\n");
        return NULL;
    }

    name = parser->current_token;

    parser_advance(parser);
    if (parser->current_token.type != TOKEN_COLON) {
        fprintf(stderr, "parse_var_decl: expected ':' after property name\n");
        return NULL;
    }

    parser_advance(parser);
    if (!parser_is_valid_type_token(parser->current_token.type)) {
        fprintf(stderr, "parse_var_decl: expected a valid property type\n");
        return NULL;
    }

    declared_type = parser->current_token;

    parser_advance(parser);
    if (parser->current_token.type != TOKEN_SEMICOLON) {
        fprintf(stderr, "parse_var_decl: expected ';' after property type\n");
        return NULL;
    }

    var_node = ast_create_node(AST_VAR_DECL);
    if (var_node == NULL) {
        return NULL;
    }

    var_node->var_decl.name = name;
    var_node->var_decl.declared_type = declared_type;
    var_node->var_decl.initializer = NULL;
    return var_node;
}

static AstNode *parse_alloc_expr(Parser *parser) {
    AstNode *alloc_node = NULL;

    if (parser == NULL) {
        fprintf(stderr, "parse_alloc_expr: parser is NULL\n");
        return NULL;
    }

    if (parser->current_token.type != TOKEN_ALLOC) {
        fprintf(stderr, "parse_alloc_expr: expected TOKEN_ALLOC\n");
        return NULL;
    }

    parser_advance(parser);
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "parse_alloc_expr: expected allocated type name after 'alloc'\n");
        return NULL;
    }

    alloc_node = ast_create_node(AST_ALLOC_EXPR);
    if (alloc_node == NULL) {
        return NULL;
    }

    alloc_node->alloc_expr.type_name = parser->current_token;
    alloc_node->alloc_expr.size_expr = NULL;
    return alloc_node;
}

static AstNode *parse_statement(Parser *parser) {
    AstNode *assignment = NULL;
    AstNode *target = NULL;
    AstNode *value = NULL;
    Token target_name;
    Token empty_type;

    if (parser == NULL) {
        fprintf(stderr, "parse_statement: parser is NULL\n");
        return NULL;
    }

    /* Statement grammar for this phase: IDENTIFIER (':')? '=' alloc IDENTIFIER ';' */
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "parse_statement: expected statement starting with IDENTIFIER\n");
        return NULL;
    }

    target_name = parser->current_token;

    parser_advance(parser);
    if (parser->current_token.type == TOKEN_COLON) {
        parser_advance(parser);
        if (parser->current_token.type != TOKEN_ASSIGN) {
            fprintf(stderr, "parse_statement: expected '=' after ':' in assignment\n");
            return NULL;
        }
    } else if (parser->current_token.type != TOKEN_ASSIGN) {
        fprintf(stderr, "parse_statement: expected assignment operator ':=' or '='\n");
        return NULL;
    }

    parser_advance(parser);
    if (parser->current_token.type == TOKEN_ALLOC) {
        value = parse_alloc_expr(parser);
        if (value == NULL) {
            return NULL;
        }
    } else {
        fprintf(stderr, "parse_statement: expected alloc expression on assignment RHS\n");
        return NULL;
    }

    parser_advance(parser);
    if (parser->current_token.type != TOKEN_SEMICOLON) {
        fprintf(stderr, "parse_statement: expected ';' at end of statement\n");
        free(value);
        return NULL;
    }

    target = ast_create_node(AST_VAR_DECL);
    if (target == NULL) {
        free(value);
        return NULL;
    }

    empty_type.start = NULL;
    empty_type.length = 0u;
    empty_type.type = TOKEN_EOF;

    target->var_decl.name = target_name;
    target->var_decl.declared_type = empty_type;
    target->var_decl.initializer = NULL;

    assignment = ast_create_node(AST_ASSIGNMENT);
    if (assignment == NULL) {
        free(target);
        free(value);
        return NULL;
    }

    assignment->assignment.target = target;
    assignment->assignment.value = value;
    return assignment;
}

static AstNode *parse_class_decl(Parser *parser) {
    AstNode *class_node = NULL;

    /* Recursive descent entry: class_decl := 'class' IDENTIFIER '{' ... '}' */
    if (parser->current_token.type != TOKEN_CLASS) {
        fprintf(stderr, "parse_class_decl: expected TOKEN_CLASS\n");
        return NULL;
    }

    class_node = ast_create_node(AST_CLASS_DECL);
    if (class_node == NULL) {
        return NULL;
    }

    parser_advance(parser);
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "parse_class_decl: expected class name after 'class'\n");
        free(class_node);
        return NULL;
    }

    class_node->class_decl.name = parser->current_token;

    parser_advance(parser);
    if (parser->current_token.type != TOKEN_LBRACE) {
        fprintf(stderr, "parse_class_decl: expected '{' after class name\n");
        free(class_node);
        return NULL;
    }

    parser_advance(parser);
    while (parser->current_token.type != TOKEN_RBRACE &&
           parser->current_token.type != TOKEN_EOF) {
        if (parser->current_token.type == TOKEN_IDENTIFIER) {
            AstNode *member = parse_var_decl(parser);
            if (member != NULL) {
                if (!parser_append_class_member(&class_node->class_decl, member)) {
                    free(member);
                    free(class_node->class_decl.members);
                    free(class_node);
                    return NULL;
                }

                /* parse_var_decl exits on ';', so advance to the next token. */
                parser_advance(parser);
                continue;
            }

            fprintf(stderr, "parse_class_decl: invalid property declaration near '%.*s'\n",
                    (int)parser->current_token.length,
                    parser->current_token.start);
            parser_advance(parser);
            continue;
        }

        fprintf(stderr, "parse_class_decl: unexpected token in class body: '%.*s'\n",
                (int)parser->current_token.length,
                parser->current_token.start);
        parser_advance(parser);
    }

    if (parser->current_token.type == TOKEN_EOF) {
        fprintf(stderr, "parse_class_decl: unexpected EOF while scanning class body\n");
        free(class_node->class_decl.members);
        free(class_node);
        return NULL;
    }

    return class_node;
}

static AstNode *parse_func_decl(Parser *parser) {
    AstNode *func_node = NULL;
    Token default_return_type;

    /* Recursive descent entry: func_decl := 'fn' IDENTIFIER '(' params? ')' (':' type)? '{' statements* '}' */
    if (parser->current_token.type != TOKEN_FN) {
        fprintf(stderr, "parse_func_decl: expected TOKEN_FN\n");
        return NULL;
    }

    func_node = ast_create_node(AST_FUNC_DECL);
    if (func_node == NULL) {
        return NULL;
    }

    parser_advance(parser);
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "parse_func_decl: expected function name after 'fn'\n");
        free(func_node);
        return NULL;
    }
    func_node->func_decl.name = parser->current_token;

    parser_advance(parser);
    if (parser->current_token.type != TOKEN_LPAREN) {
        fprintf(stderr, "parse_func_decl: expected '(' after function name\n");
        free(func_node);
        return NULL;
    }

    parser_advance(parser);
    while (parser->current_token.type != TOKEN_RPAREN) {
        AstNode *parameter = NULL;
        Token param_name;
        Token param_type;

        if (parser->current_token.type == TOKEN_EOF) {
            fprintf(stderr, "parse_func_decl: unexpected EOF in parameter list\n");
            free(func_node->func_decl.parameters);
            free(func_node);
            return NULL;
        }

        if (parser->current_token.type != TOKEN_IDENTIFIER) {
            fprintf(stderr, "parse_func_decl: expected parameter name (IDENTIFIER)\n");
            free(func_node->func_decl.parameters);
            free(func_node);
            return NULL;
        }
        param_name = parser->current_token;

        parser_advance(parser);
        if (parser->current_token.type != TOKEN_COLON) {
            fprintf(stderr, "parse_func_decl: expected ':' after parameter name\n");
            free(func_node->func_decl.parameters);
            free(func_node);
            return NULL;
        }

        parser_advance(parser);
        if (!parser_is_valid_type_token(parser->current_token.type)) {
            fprintf(stderr, "parse_func_decl: expected valid parameter type\n");
            free(func_node->func_decl.parameters);
            free(func_node);
            return NULL;
        }
        param_type = parser->current_token;

        parameter = ast_create_node(AST_VAR_DECL);
        if (parameter == NULL) {
            free(func_node->func_decl.parameters);
            free(func_node);
            return NULL;
        }
        parameter->var_decl.name = param_name;
        parameter->var_decl.declared_type = param_type;
        parameter->var_decl.initializer = NULL;

        if (!parser_append_func_parameter(&func_node->func_decl, parameter)) {
            free(parameter);
            free(func_node->func_decl.parameters);
            free(func_node);
            return NULL;
        }

        parser_advance(parser);
        if (parser->current_token.type == TOKEN_RPAREN) {
            break;
        }

        if (!parser_is_comma_token(&parser->current_token)) {
            fprintf(stderr, "parse_func_decl: expected ',' or ')' after parameter\n");
            free(func_node->func_decl.parameters);
            free(func_node);
            return NULL;
        }

        parser_advance(parser);
    }

    default_return_type.start = NULL;
    default_return_type.length = 0u;
    default_return_type.type = TOKEN_EOF;
    func_node->func_decl.return_type = default_return_type;

    parser_advance(parser);
    if (parser->current_token.type == TOKEN_COLON) {
        parser_advance(parser);
        if (!parser_is_valid_type_token(parser->current_token.type)) {
            fprintf(stderr, "parse_func_decl: expected valid return type after ':'\n");
            free(func_node->func_decl.parameters);
            free(func_node);
            return NULL;
        }
        func_node->func_decl.return_type = parser->current_token;
        parser_advance(parser);
    }

    if (parser->current_token.type != TOKEN_LBRACE) {
        fprintf(stderr, "parse_func_decl: expected '{' after function signature\n");
        free(func_node->func_decl.parameters);
        free(func_node);
        return NULL;
    }

    parser_advance(parser);
    while (parser->current_token.type != TOKEN_RBRACE &&
           parser->current_token.type != TOKEN_EOF) {
        AstNode *statement = parse_statement(parser);
        if (statement == NULL) {
            fprintf(stderr, "parse_func_decl: failed to parse statement near '%.*s'\n",
                    (int)parser->current_token.length,
                    parser->current_token.start);
            free(func_node->func_decl.parameters);
            free(func_node->func_decl.body_statements);
            free(func_node);
            return NULL;
        }

        if (!parser_append_node_ptr(
                &func_node->func_decl.body_statements,
                &func_node->func_decl.body_statement_count,
                statement,
                "parse_func_decl")) {
            free(statement);
            free(func_node->func_decl.parameters);
            free(func_node->func_decl.body_statements);
            free(func_node);
            return NULL;
        }

        /* parse_statement exits on ';', so advance to the next statement token. */
        parser_advance(parser);
    }

    if (parser->current_token.type == TOKEN_EOF) {
        fprintf(stderr, "parse_func_decl: unexpected EOF while scanning function body\n");
        free(func_node->func_decl.parameters);
        free(func_node->func_decl.body_statements);
        free(func_node);
        return NULL;
    }

    return func_node;
}

void parser_init(Parser *parser, const char *source) {
    if (parser == NULL) {
        return;
    }

    parser->current_source_pointer = source;

    if (source == NULL) {
        parser->current_token.start = NULL;
        parser->current_token.length = 0u;
        parser->current_token.type = TOKEN_EOF;
        parser->peek_token = parser->current_token;
        return;
    }

    parser->current_token = lexer_next_token(&parser->current_source_pointer);
    parser->peek_token = lexer_next_token(&parser->current_source_pointer);
}

AstNode *parse_program(Parser *parser) {
    AstNode *program_node = NULL;

    if (parser == NULL) {
        fprintf(stderr, "parse_program: parser is NULL\n");
        return NULL;
    }

    program_node = ast_create_node(AST_PROGRAM);
    if (program_node == NULL) {
        return NULL;
    }

    /*
     * Top-level recursive descent loop.
     * Dispatch by current token and always guarantee forward progress.
     */
    while (parser->current_token.type != TOKEN_EOF) {
        if (parser->current_token.type == TOKEN_CLASS) {
            AstNode *class_decl = parse_class_decl(parser);
            if (class_decl == NULL) {
                free(program_node->program.declarations);
                free(program_node);
                return NULL;
            }

            if (!parser_append_declaration(&program_node->program, class_decl)) {
                free(class_decl);
                free(program_node->program.declarations);
                free(program_node);
                return NULL;
            }

            /* parse_class_decl exits with current token on the closing '}'. */
            parser_advance(parser);
            continue;
        }

        if (parser->current_token.type == TOKEN_FN) {
            AstNode *func_decl = parse_func_decl(parser);
            if (func_decl == NULL) {
                free(program_node->program.declarations);
                free(program_node);
                return NULL;
            }

            if (!parser_append_declaration(&program_node->program, func_decl)) {
                free(func_decl);
                free(program_node->program.declarations);
                free(program_node);
                return NULL;
            }

            /* parse_func_decl exits with current token on the closing '}'. */
            parser_advance(parser);
            continue;
        }

        /* Unknown top-level token for this phase: skip and continue scanning. */
        parser_advance(parser);
    }

    return program_node;
}
