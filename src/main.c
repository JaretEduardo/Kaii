#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "utils/file_io.h"

int main(int argc, char *argv[]) {
    const char *source_path = NULL;
    char *source = NULL;
    size_t source_size = 0u;

    if (argc != 3 || strcmp(argv[1], "build") != 0) {
        fprintf(stderr, "Kaii Compiler (Stage 1)\n");
        fprintf(stderr, "Usage: %s build <file.kaii>\n", argv[0]);
        return EXIT_FAILURE;
    }

    source_path = argv[2];
    source = read_file_to_string(source_path, &source_size);
    if (source == NULL) {
        return EXIT_FAILURE;
    }

    printf("[*] Loading %s (%zu bytes)\n", source_path, source_size);
    printf("[*] Starting Parser...\n");

    Parser parser;
    parser_init(&parser, source);

    AstNode *ast = parse_program(&parser);

    if (ast == NULL) {
        printf("\n[!] Fatal syntax error. Parser stopped.\n");
    } else {
        printf("\n[+] Syntax analysis completed successfully.\n");
        printf("[+] The file contains %zu global declarations.\n", ast->program.declaration_count);
        
        // We print the names of the classes and their properties
        printf("\n--- AST STRUCTURE ---\n");
        for (size_t i = 0; i < ast->program.declaration_count; i++) {
            AstNode *decl = ast->program.declarations[i];
            
            if (decl->type == AST_CLASS_DECL) {
                printf(" -> class %.*s {\n", 
                       (int)decl->class_decl.name.length, 
                       decl->class_decl.name.start);
                
                // We iterate through the properties (variables) of the class
                for (size_t j = 0; j < decl->class_decl.member_count; j++) {
                    AstNode *member = decl->class_decl.members[j];
                    if (member->type == AST_VAR_DECL) {
                        printf("      property: %.*s | type: %.*s\n",
                               (int)member->var_decl.name.length,
                               member->var_decl.name.start,
                               (int)member->var_decl.declared_type.length,
                               member->var_decl.declared_type.start);
                    }
                }
                printf("    }\n");
            }

            else if (decl->type == AST_FUNC_DECL) {
                printf(" -> fn %.*s() {\n",
                       (int)decl->func_decl.name.length,
                       decl->func_decl.name.start);
                printf("      [Body execution logic recognized]\n");
                printf("    }\n");
            }
        }
    }

    free(source);
    return (ast == NULL) ? EXIT_FAILURE : EXIT_SUCCESS;
}