#ifndef TINYSCRIPT_IR_H
#define TINYSCRIPT_IR_H

#include <stdio.h>
#include "ast.h"

typedef enum {
    IR_PROGRAM,
    IR_DEVICE,
    IR_ACTION,
    IR_ASSIGN,
    IR_IF,
    IR_LOOP,
    IR_EVENT,
    IR_SCHEDULE,
    IR_LOG
} IRKind;

typedef struct IRNode {
    IRKind kind;
    int line;
    char *name;
    char *message;
    Expr *expr;
    struct IRNode *body;
    struct IRNode *else_branch;
    struct IRNode *next;
    int times;
    int time;
    int action_on;
} IRNode;

IRNode *ir_from_ast(ASTNode *ast);
int semantic_check(ASTNode *ast);
IRNode *ir_optimize(IRNode *root);
void ir_print(IRNode *root, FILE *out);
int ir_write_text(const char *path, IRNode *root);
void ir_free(IRNode *root);

#endif // TINYSCRIPT_IR_H
