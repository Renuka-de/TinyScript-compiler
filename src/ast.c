#include "ast.h"
#include <stdlib.h>
#include <string.h>

static ASTNode *ast_new_node(NodeType type, int line) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    node->type = type;
    node->line = line;
    node->name = NULL;
    node->message = NULL;
    node->expr = NULL;
    node->body = NULL;
    node->else_branch = NULL;
    node->next = NULL;
    node->times = 0;
    node->time = 0;
    node->action_on = 0;
    return node;
}

ASTNode *ast_new_program(ASTNode *body) {
    ASTNode *node = ast_new_node(NODE_PROGRAM, 0);
    node->body = body;
    return node;
}

ASTNode *ast_new_device_decl(char *name, int line) {
    ASTNode *node = ast_new_node(NODE_DEVICE_DECL, line);
    node->name = strdup(name);
    return node;
}

ASTNode *ast_new_action(char *name, int on, int line) {
    ASTNode *node = ast_new_node(NODE_ACTION, line);
    node->name = strdup(name);
    node->action_on = on;
    return node;
}

ASTNode *ast_new_assign(char *name, Expr *expr, int line) {
    ASTNode *node = ast_new_node(NODE_ASSIGN, line);
    node->name = strdup(name);
    node->expr = expr;
    return node;
}

ASTNode *ast_new_if(Expr *cond, ASTNode *then_branch, ASTNode *else_branch, int line) {
    ASTNode *node = ast_new_node(NODE_IF, line);
    node->expr = cond;
    node->body = then_branch;
    node->else_branch = else_branch;
    return node;
}

ASTNode *ast_new_loop(int times, ASTNode *body, int line) {
    ASTNode *node = ast_new_node(NODE_LOOP, line);
    node->times = times;
    node->body = body;
    return node;
}

ASTNode *ast_new_event(Expr *cond, ASTNode *body, int line) {
    ASTNode *node = ast_new_node(NODE_EVENT, line);
    node->expr = cond;
    node->body = body;
    return node;
}

ASTNode *ast_new_schedule(int time, ASTNode *body, int line) {
    ASTNode *node = ast_new_node(NODE_SCHEDULE, line);
    node->time = time;
    node->body = body;
    return node;
}

ASTNode *ast_new_log(char *message, int line) {
    ASTNode *node = ast_new_node(NODE_LOG, line);
    node->message = strdup(message);
    return node;
}

ASTNode *ast_append_stmt(ASTNode *list, ASTNode *node) {
    if (!list) return node;
    ASTNode *cursor = list;
    while (cursor->next) cursor = cursor->next;
    cursor->next = node;
    return list;
}

Expr *ast_new_number_expr(int number) {
    Expr *expr = (Expr *)malloc(sizeof(Expr));
    expr->kind = EXPR_NUMBER;
    expr->number = number;
    expr->name = NULL;
    expr->left = expr->right = NULL;
    return expr;
}

Expr *ast_new_variable_expr(char *name) {
    Expr *expr = (Expr *)malloc(sizeof(Expr));
    expr->kind = EXPR_VAR;
    expr->name = strdup(name);
    expr->left = expr->right = NULL;
    return expr;
}

Expr *ast_new_binary_expr(BinaryOp op, Expr *left, Expr *right) {
    Expr *expr = (Expr *)malloc(sizeof(Expr));
    expr->kind = EXPR_BINARY;
    expr->op = op;
    expr->left = left;
    expr->right = right;
    expr->name = NULL;
    return expr;
}

void expr_free(Expr *expr) {
    if (!expr) return;
    if (expr->kind == EXPR_VAR && expr->name) free(expr->name);
    if (expr->left) expr_free(expr->left);
    if (expr->right) expr_free(expr->right);
    free(expr);
}

void ast_free(ASTNode *node) {
    if (!node) return;
    if (node->name) free(node->name);
    if (node->message) free(node->message);
    if (node->expr) expr_free(node->expr);
    if (node->body) ast_free(node->body);
    if (node->else_branch) ast_free(node->else_branch);
    if (node->next) ast_free(node->next);
    free(node);
}

static void ast_print_indent(int indent, FILE *out) {
    for (int i = 0; i < indent; ++i) fputc(' ', out);
}

static void expr_print(Expr *expr, FILE *out) {
    if (!expr) return;
    switch (expr->kind) {
        case EXPR_NUMBER:
            fprintf(out, "%d", expr->number);
            break;
        case EXPR_VAR:
            fprintf(out, "%s", expr->name);
            break;
        case EXPR_SENSOR:
            fprintf(out, "%s", expr->name);
            break;
        case EXPR_BINARY:
            fprintf(out, "(");
            expr_print(expr->left, out);
            switch (expr->op) {
                case OP_ADD: fprintf(out, " + "); break;
                case OP_SUB: fprintf(out, " - "); break;
                case OP_MUL: fprintf(out, " * "); break;
                case OP_DIV: fprintf(out, " / "); break;
                case OP_GT: fprintf(out, " > "); break;
                case OP_LT: fprintf(out, " < "); break;
                case OP_GE: fprintf(out, " >= "); break;
                case OP_LE: fprintf(out, " <= "); break;
                case OP_EQ: fprintf(out, " == "); break;
                case OP_NEQ: fprintf(out, " != "); break;
            }
            expr_print(expr->right, out);
            fprintf(out, ")");
            break;
    }
}

void ast_print(ASTNode *node, int indent, FILE *out) {
    while (node) {
        ast_print_indent(indent, out);
        switch (node->type) {
            case NODE_PROGRAM:
                fprintf(out, "PROGRAM\n");
                if (node->body) ast_print(node->body, indent + 2, out);
                break;
            case NODE_DEVICE_DECL:
                fprintf(out, "DEVICE %s\n", node->name);
                break;
            case NODE_ACTION:
                fprintf(out, "ACTION %s %s\n", node->name, node->action_on ? "ON" : "OFF");
                break;
            case NODE_ASSIGN:
                fprintf(out, "ASSIGN %s = ", node->name);
                expr_print(node->expr, out);
                fprintf(out, "\n");
                break;
            case NODE_IF:
                fprintf(out, "IF "); expr_print(node->expr, out); fprintf(out, " THEN\n");
                if (node->body) ast_print(node->body, indent + 2, out);
                if (node->else_branch) {
                    ast_print_indent(indent, out);
                    fprintf(out, "ELSE\n");
                    ast_print(node->else_branch, indent + 2, out);
                }
                break;
            case NODE_LOOP:
                fprintf(out, "LOOP %d TIMES\n", node->times);
                if (node->body) ast_print(node->body, indent + 2, out);
                break;
            case NODE_EVENT:
                fprintf(out, "ON "); expr_print(node->expr, out); fprintf(out, " THEN\n");
                if (node->body) ast_print(node->body, indent + 2, out);
                break;
            case NODE_SCHEDULE:
                fprintf(out, "SCHEDULE AT %02d:%02d\n", node->time / 60, node->time % 60);
                if (node->body) ast_print(node->body, indent + 2, out);
                break;
            case NODE_LOG:
                fprintf(out, "LOG \"%s\"\n", node->message);
                break;
        }
        node = node->next;
    }
}

int ast_parse_time(const char *text) {
    int hour = 0, minute = 0;
    if (sscanf(text, "%d:%d", &hour, &minute) != 2) return -1;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return -1;
    return hour * 60 + minute;
}
