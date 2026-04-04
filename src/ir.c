#include "ir.h"
#include <stdlib.h>
#include <string.h>

static IRNode *ir_new_node(IRKind kind, int line) {
    IRNode *node = (IRNode *)malloc(sizeof(IRNode));
    node->kind = kind;
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

static IRNode *ir_clone_node(ASTNode *ast) {
    if (!ast) return NULL;
    IRNode *node = ir_new_node(ast->type == NODE_PROGRAM ? IR_PROGRAM : (IRKind)ast->type, ast->line);
    if (ast->name) node->name = strdup(ast->name);
    if (ast->message) node->message = strdup(ast->message);
    node->action_on = ast->action_on;
    node->times = ast->times;
    node->time = ast->time;
    if (ast->expr) {
        node->expr = ast->expr; // reuse expressions directly
        ast->expr = NULL;
    }
    return node;
}

static IRNode *ir_clone_list(ASTNode *ast) {
    IRNode *head = NULL;
    IRNode *tail = NULL;
    while (ast) {
        IRNode *node = ir_clone_node(ast);
        node->body = ir_clone_list(ast->body);
        node->else_branch = ir_clone_list(ast->else_branch);
        node->next = NULL;
        if (!head) {
            head = tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
        ast = ast->next;
    }
    return head;
}

IRNode *ir_from_ast(ASTNode *ast) {
    if (!ast) return NULL;
    IRNode *program = ir_new_node(IR_PROGRAM, ast->line);
    program->body = ir_clone_list(ast->body);
    return program;
}

#define MAX_SYMBOLS 128

typedef struct {
    char *names[MAX_SYMBOLS];
    int count;
} SymbolSet;

static void symbol_add(SymbolSet *set, const char *name) {
    if (!set || !name || set->count >= MAX_SYMBOLS) return;
    for (int i = 0; i < set->count; ++i) {
        if (strcmp(set->names[i], name) == 0) return;
    }
    set->names[set->count++] = strdup(name);
}

static int symbol_has(SymbolSet *set, const char *name) {
    if (!set || !name) return 0;
    for (int i = 0; i < set->count; ++i) {
        if (strcmp(set->names[i], name) == 0) return 1;
    }
    return 0;
}

static int is_sensor_name(const char *name) {
    return name && (
        strcmp(name, "temperature") == 0 ||
        strcmp(name, "humidity") == 0 ||
        strcmp(name, "light") == 0 ||
        strcmp(name, "motion") == 0
    );
}

static int check_expr(Expr *expr, SymbolSet *vars, SymbolSet *devices) {
    if (!expr) return 1;
    switch (expr->kind) {
        case EXPR_NUMBER:
            return 1;
        case EXPR_VAR:
            if (is_sensor_name(expr->name)) return 1;
            if (symbol_has(vars, expr->name)) return 1;
            fprintf(stderr, "Semantic error: undefined symbol '%s'\n", expr->name);
            return 0;
        case EXPR_BINARY:
            return check_expr(expr->left, vars, devices) && check_expr(expr->right, vars, devices);
        default:
            return 1;
    }
}

static int semantic_check_node(ASTNode *node, SymbolSet *devices, SymbolSet *vars);

static int semantic_check_stmt_list(ASTNode *node, SymbolSet *devices, SymbolSet *vars) {
    int ok = 1;
    while (node) {
        ok &= semantic_check_node(node, devices, vars);
        node = node->next;
    }
    return ok;
}

static int semantic_check_node(ASTNode *node, SymbolSet *devices, SymbolSet *vars) {
    if (!node) return 1;
    switch (node->type) {
        case NODE_DEVICE_DECL:
            symbol_add(devices, node->name);
            return 1;
        case NODE_ACTION:
            if (!symbol_has(devices, node->name)) {
                fprintf(stderr, "Semantic error at line %d: device '%s' not declared\n", node->line, node->name);
                return 0;
            }
            return 1;
        case NODE_ASSIGN:
            if (!check_expr(node->expr, vars, devices)) return 0;
            symbol_add(vars, node->name);
            return 1;
        case NODE_IF:
            if (!check_expr(node->expr, vars, devices)) return 0;
            return semantic_check_stmt_list(node->body, devices, vars) && semantic_check_stmt_list(node->else_branch, devices, vars);
        case NODE_LOOP:
            if (node->times < 0) {
                fprintf(stderr, "Semantic error at line %d: negative loop count\n", node->line);
                return 0;
            }
            return semantic_check_stmt_list(node->body, devices, vars);
        case NODE_EVENT:
            if (!check_expr(node->expr, vars, devices)) return 0;
            return semantic_check_stmt_list(node->body, devices, vars);
        case NODE_SCHEDULE:
            if (node->time < 0) {
                fprintf(stderr, "Semantic error at line %d: invalid schedule time\n", node->line);
                return 0;
            }
            return semantic_check_stmt_list(node->body, devices, vars);
        case NODE_LOG:
            return 1;
        case NODE_PROGRAM:
            return semantic_check_stmt_list(node->body, devices, vars);
        default:
            return 1;
    }
}

int semantic_check(ASTNode *ast) {
    SymbolSet devices = { .count = 0 };
    SymbolSet vars = { .count = 0 };
    return semantic_check_node(ast, &devices, &vars);
}

static Expr *fold_expr(Expr *expr, int *ok) {
    if (!expr) return NULL;
    if (expr->kind != EXPR_BINARY) return expr;
    expr->left = fold_expr(expr->left, ok);
    expr->right = fold_expr(expr->right, ok);
    if (expr->left && expr->right && expr->left->kind == EXPR_NUMBER && expr->right->kind == EXPR_NUMBER) {
        int a = expr->left->number;
        int b = expr->right->number;
        int value = 0;
        switch (expr->op) {
            case OP_ADD: value = a + b; break;
            case OP_SUB: value = a - b; break;
            case OP_MUL: value = a * b; break;
            case OP_DIV: value = b != 0 ? a / b : 0; break;
            case OP_GT: value = a > b; break;
            case OP_LT: value = a < b; break;
            case OP_GE: value = a >= b; break;
            case OP_LE: value = a <= b; break;
            case OP_EQ: value = a == b; break;
            case OP_NEQ: value = a != b; break;
        }
        expr_free(expr);
        return ast_new_number_expr(value);
    }
    return expr;
}

static IRNode *ir_optimize_node(IRNode *node, int *changed) {
    if (!node) return NULL;
    node->body = ir_optimize_node(node->body, changed);
    node->else_branch = ir_optimize_node(node->else_branch, changed);
    node->next = ir_optimize_node(node->next, changed);
    if (node->expr) {
        node->expr = fold_expr(node->expr, changed);
    }
    if (node->kind == IR_IF && node->expr && node->expr->kind == EXPR_NUMBER) {
        int cond = node->expr->number;
        *changed = 1;
        IRNode *replacement = cond ? node->body : node->else_branch;
        if (!replacement) {
            replacement = ir_new_node(IR_PROGRAM, node->line);
        }
        replacement->next = node->next;
        node->next = NULL;
        node->body = NULL;
        node->else_branch = NULL;
        ir_free(node);
        return replacement;
    }
    if (node->kind == IR_LOOP && node->times == 0) {
        *changed = 1;
        IRNode *replacement = node->next;
        node->next = NULL;
        ir_free(node);
        return replacement;
    }
    return node;
}

IRNode *ir_optimize(IRNode *root) {
    int changed = 0;
    do {
        changed = 0;
        root = ir_optimize_node(root, &changed);
    } while (changed);
    return root;
}

static void ir_print_indent(int indent, FILE *out) {
    for (int i = 0; i < indent; ++i) fputc(' ', out);
}

static void print_expr(Expr *expr, FILE *out) {
    if (!expr) return;
    switch (expr->kind) {
        case EXPR_NUMBER: fprintf(out, "%d", expr->number); break;
        case EXPR_VAR: fprintf(out, "%s", expr->name); break;
        case EXPR_SENSOR: fprintf(out, "%s", expr->name); break;
        case EXPR_BINARY:
            fprintf(out, "(");
            print_expr(expr->left, out);
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
            print_expr(expr->right, out);
            fprintf(out, ")");
            break;
    }
}

void ir_print(IRNode *root, FILE *out) {
    while (root) {
        ir_print_indent(2, out);
        switch (root->kind) {
            case IR_PROGRAM:
                fprintf(out, "PROGRAM\n");
                if (root->body) ir_print(root->body, out);
                break;
            case IR_DEVICE:
                fprintf(out, "DEVICE %s\n", root->name);
                break;
            case IR_ACTION:
                fprintf(out, "ACTION %s %s\n", root->name, root->action_on ? "ON" : "OFF");
                break;
            case IR_ASSIGN:
                fprintf(out, "ASSIGN %s = ", root->name);
                print_expr(root->expr, out);
                fprintf(out, "\n");
                break;
            case IR_IF:
                fprintf(out, "IF "); print_expr(root->expr, out); fprintf(out, " THEN\n");
                if (root->body) ir_print(root->body, out);
                if (root->else_branch) {
                    ir_print_indent(2, out);
                    fprintf(out, "ELSE\n");
                    ir_print(root->else_branch, out);
                }
                break;
            case IR_LOOP:
                fprintf(out, "LOOP %d TIMES\n", root->times);
                if (root->body) ir_print(root->body, out);
                break;
            case IR_EVENT:
                fprintf(out, "ON "); print_expr(root->expr, out); fprintf(out, " THEN\n");
                if (root->body) ir_print(root->body, out);
                break;
            case IR_SCHEDULE:
                fprintf(out, "SCHEDULE AT %02d:%02d\n", root->time / 60, root->time % 60);
                if (root->body) ir_print(root->body, out);
                break;
            case IR_LOG:
                fprintf(out, "LOG \"%s\"\n", root->message);
                break;
        }
        root = root->next;
    }
}

int ir_write_text(const char *path, IRNode *root) {
    FILE *out = fopen(path, "w");
    if (!out) return 0;
    ir_print(root, out);
    fclose(out);
    return 1;
}

void ir_free(IRNode *root) {
    while (root) {
        IRNode *next = root->next;
        if (root->name) free(root->name);
        if (root->message) free(root->message);
        if (root->expr) expr_free(root->expr);
        if (root->body) ir_free(root->body);
        if (root->else_branch) ir_free(root->else_branch);
        free(root);
        root = next;
    }
}
