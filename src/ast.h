#ifndef TINYSCRIPT_AST_H
#define TINYSCRIPT_AST_H

#include <stdio.h>

typedef enum {
    NODE_PROGRAM,
    NODE_DEVICE_DECL,
    NODE_ACTION,
    NODE_ASSIGN,
    NODE_IF,
    NODE_LOOP,
    NODE_EVENT,
    NODE_SCHEDULE,
    NODE_LOG
} NodeType;

typedef enum {
    EXPR_NUMBER,
    EXPR_VAR,
    EXPR_SENSOR,
    EXPR_BINARY
} ExprKind;

typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_GT,
    OP_LT,
    OP_GE,
    OP_LE,
    OP_EQ,
    OP_NEQ
} BinaryOp;

typedef struct Expr {
    ExprKind kind;
    int number;
    char *name;
    BinaryOp op;
    struct Expr *left;
    struct Expr *right;
} Expr;

typedef struct ASTNode {
    NodeType type;
    int line;
    char *name;
    char *message;
    Expr *expr;
    struct ASTNode *body;
    struct ASTNode *else_branch;
    struct ASTNode *next;
    int times;
    int time;
    int action_on;
} ASTNode;

ASTNode *ast_new_program(ASTNode *body);
ASTNode *ast_new_device_decl(char *name, int line);
ASTNode *ast_new_action(char *name, int on, int line);
ASTNode *ast_new_assign(char *name, Expr *expr, int line);
ASTNode *ast_new_if(Expr *cond, ASTNode *then_branch, ASTNode *else_branch, int line);
ASTNode *ast_new_loop(int times, ASTNode *body, int line);
ASTNode *ast_new_event(Expr *cond, ASTNode *body, int line);
ASTNode *ast_new_schedule(int time, ASTNode *body, int line);
ASTNode *ast_new_log(char *message, int line);
ASTNode *ast_append_stmt(ASTNode *list, ASTNode *node);
Expr *ast_new_number_expr(int number);
Expr *ast_new_variable_expr(char *name);
Expr *ast_new_binary_expr(BinaryOp op, Expr *left, Expr *right);
void ast_free(ASTNode *node);
void expr_free(Expr *expr);
void ast_print(ASTNode *node, int indent, FILE *out);
int ast_parse_time(const char *text);

#endif // TINYSCRIPT_AST_H
