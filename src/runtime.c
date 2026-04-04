#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

#define MAX_SYMBOLS 64

static char *device_names[MAX_SYMBOLS];
static int device_states[MAX_SYMBOLS];
static int device_count = 0;

static char *variable_names[MAX_SYMBOLS];
static int variable_values[MAX_SYMBOLS];
static int variable_count = 0;

static IRNode *event_root = NULL;
static IRNode *schedule_root = NULL;

static int current_time = 0;

static int find_device(const char *name) {
    for (int i = 0; i < device_count; ++i) {
        if (strcmp(device_names[i], name) == 0) return i;
    }
    return -1;
}

static int find_variable(const char *name) {
    for (int i = 0; i < variable_count; ++i) {
        if (strcmp(variable_names[i], name) == 0) return i;
    }
    return -1;
}

static int is_sensor(const char *name) {
    return strcmp(name, "temperature") == 0 || strcmp(name, "humidity") == 0 || strcmp(name, "light") == 0 || strcmp(name, "motion") == 0;
}

static void register_device(const char *name) {
    if (find_device(name) >= 0) return;
    if (device_count >= MAX_SYMBOLS) return;
    device_names[device_count] = strdup(name);
    device_states[device_count] = 0;
    device_count++;
}

static void set_variable(const char *name, int value) {
    int idx = find_variable(name);
    if (idx < 0) {
        if (variable_count >= MAX_SYMBOLS) return;
        variable_names[variable_count] = strdup(name);
        variable_values[variable_count] = value;
        variable_count++;
        return;
    }
    variable_values[idx] = value;
}

static int get_sensor_value(const char *name, int minute) {
    double t = minute / 1440.0;
    if (strcmp(name, "temperature") == 0) {
        return 18 + (int)(8.0 * sin(t * 2.0 * M_PI) + 0.5);
    }
    if (strcmp(name, "humidity") == 0) {
        return 48 + (int)(12.0 * cos((t + 0.2) * 2.0 * M_PI));
    }
    if (strcmp(name, "light") == 0) {
        return (minute >= 360 && minute < 1080) ? 900 : 20;
    }
    if (strcmp(name, "motion") == 0) {
        return ((minute % 180) < 20) ? 1 : 0;
    }
    return 0;
}

static int evaluate_expr(Expr *expr) {
    if (!expr) return 0;
    switch (expr->kind) {
        case EXPR_NUMBER:
            return expr->number;
        case EXPR_VAR: {
            int idx = find_variable(expr->name);
            if (idx >= 0) return variable_values[idx];
            if (is_sensor(expr->name)) return get_sensor_value(expr->name, current_time);
            return 0;
        }
        case EXPR_SENSOR:
            return get_sensor_value(expr->name, current_time);
        case EXPR_BINARY: {
            int left = evaluate_expr(expr->left);
            int right = evaluate_expr(expr->right);
            switch (expr->op) {
                case OP_ADD: return left + right;
                case OP_SUB: return left - right;
                case OP_MUL: return left * right;
                case OP_DIV: return right != 0 ? left / right : 0;
                case OP_GT: return left > right;
                case OP_LT: return left < right;
                case OP_GE: return left >= right;
                case OP_LE: return left <= right;
                case OP_EQ: return left == right;
                case OP_NEQ: return left != right;
            }
        }
    }
    return 0;
}

static void log_message(const char *message) {
    printf("[time %02d:%02d] %s\n", current_time / 60, current_time % 60, message);
}

static void set_device_state(const char *name, int on) {
    int idx = find_device(name);
    if (idx < 0) {
        printf("[ERROR] Undefined device '%s'\n", name);
        return;
    }
    device_states[idx] = on;
    printf("[time %02d:%02d] Device '%s' turned %s\n", current_time / 60, current_time % 60, name, on ? "ON" : "OFF");
}

static void execute_block(IRNode *node);

static void execute_node(IRNode *node) {
    if (!node) return;
    switch (node->kind) {
        case IR_DEVICE:
            register_device(node->name);
            break;
        case IR_ACTION:
            set_device_state(node->name, node->action_on);
            break;
        case IR_ASSIGN: {
            int value = evaluate_expr(node->expr);
            set_variable(node->name, value);
            printf("[time %02d:%02d] Variable '%s' = %d\n", current_time / 60, current_time % 60, node->name, value);
            break;
        }
        case IR_IF: {
            if (evaluate_expr(node->expr)) {
                execute_block(node->body);
            } else {
                execute_block(node->else_branch);
            }
            break;
        }
        case IR_LOOP: {
            for (int i = 0; i < node->times; ++i) {
                execute_block(node->body);
            }
            break;
        }
        case IR_EVENT:
        case IR_SCHEDULE:
            break;
        case IR_LOG:
            log_message(node->message);
            break;
        case IR_PROGRAM:
            execute_block(node->body);
            break;
    }
}

static void execute_block(IRNode *node) {
    while (node) {
        execute_node(node);
        node = node->next;
    }
}

static void execute_events_and_schedules(void) {
    int event_index = 0;
    int event_state[MAX_SYMBOLS] = {0};
    for (current_time = 0; current_time < 24 * 60; current_time += 5) {
        IRNode *scan = event_root;
        while (scan) {
            int cond = evaluate_expr(scan->expr);
            if (cond && !event_state[event_index]) {
                log_message("Event triggered");
                execute_block(scan->body);
            }
            event_state[event_index++] = cond;
            scan = scan->next;
        }
        event_index = 0;
        scan = schedule_root;
        while (scan) {
            if (scan->time == current_time) {
                log_message("Schedule activated");
                execute_block(scan->body);
            }
            scan = scan->next;
        }
    }
}

int run_simulation(IRNode *program) {
    if (!program) return 0;
    event_root = NULL;
    schedule_root = NULL;
    device_count = 0;
    variable_count = 0;
    current_time = 0;
    printf("[SIMULATION] Starting TinyScript runtime\n");
    IRNode *top = program->body;
    while (top) {
        if (top->kind == IR_EVENT || top->kind == IR_SCHEDULE) {
            IRNode *deferred = top;
            top = top->next;
            deferred->next = NULL;
            if (deferred->kind == IR_EVENT) {
                deferred->next = event_root;
                event_root = deferred;
            } else {
                deferred->next = schedule_root;
                schedule_root = deferred;
            }
            continue;
        }
        execute_node(top);
        IRNode *next = top->next;
        top = next;
    }
    execute_events_and_schedules();
    printf("[SIMULATION] Completed runtime\n");
    return 1;
}
