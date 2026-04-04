#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "ir.h"
#include "runtime.h"

extern int yyparse(void);
extern FILE *yyin;
extern ASTNode *root;

void usage(const char *name) {
    fprintf(stderr, "Usage: %s <script.ts> [--run] [--emit-ir] [--output <file>]\n", name);
    fprintf(stderr, "  --run       Execute the compiled TinyScript program\n");
    fprintf(stderr, "  --emit-ir   Write the intermediate representation to program.tsb\n");
    fprintf(stderr, "  --output    Write IR to the specified file\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    const char *input = argv[1];
    const char *output = "program.tsb";
    int run_flag = 0;
    int emit_ir = 0;

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--run") == 0) {
            run_flag = 1;
        } else if (strcmp(argv[i], "--emit-ir") == 0) {
            emit_ir = 1;
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            output = argv[++i];
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    yyin = fopen(input, "r");
    if (!yyin) {
        fprintf(stderr, "Unable to open input file: %s\n", input);
        return 1;
    }

    if (yyparse() != 0) {
        fprintf(stderr, "Parsing failed.\n");
        fclose(yyin);
        return 1;
    }
    fclose(yyin);

    if (!root) {
        fprintf(stderr, "No program parsed.\n");
        return 1;
    }

    if (!semantic_check(root)) {
        fprintf(stderr, "Semantic validation failed.\n");
        ast_free(root);
        return 1;
    }

    IRNode *program = ir_from_ast(root);
    if (!program) {
        fprintf(stderr, "IR generation failed.\n");
        ast_free(root);
        return 1;
    }

    program = ir_optimize(program);
    if (!ir_write_text(output, program)) {
        fprintf(stderr, "Failed to write IR to %s\n", output);
    }

    if (emit_ir) {
        printf("Wrote IR output to %s\n", output);
    }

    if (run_flag) {
        if (!run_simulation(program)) {
            fprintf(stderr, "Runtime execution failed.\n");
        }
    }

    ir_free(program);
    ast_free(root);
    return 0;
}
