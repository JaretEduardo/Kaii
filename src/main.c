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
        
        printf("\n--- CLASSES DETECTED ---\n");
        for (size_t i = 0; i < ast->program.declaration_count; i++) {
            AstNode *decl = ast->program.declarations[i];
            if (decl->type == AST_CLASS_DECL) {
                printf(" -> class %.*s\n", 
                       (int)decl->class_decl.name.length, 
                       decl->class_decl.name.start);
            }
        }
    }

    free(source);
    return (ast == NULL) ? EXIT_FAILURE : EXIT_SUCCESS;
}