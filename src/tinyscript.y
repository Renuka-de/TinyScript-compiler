%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

extern int yylineno;
int yylex(void);
void yyerror(const char *s);
ASTNode *root = NULL;
int is_sensor_name(const char *name);
%}

%code requires {
#include "ast.h"
}

%union {
    ASTNode *node;
    Expr *expr;
    char *str;
    int num;
}

%token <str> IDENT STRING TIME
%token <num> NUMBER
%token DEVICE IF THEN ELSE END LOOP TIMES DO ON SCHEDULE AT LOG OFF
%token PLUS MINUS MULT DIV GT LT GE LE EQ NEQ ASSIGN

%define parse.error verbose

%left EQ NEQ
%left GT LT GE LE
%left PLUS MINUS
%left MULT DIV

%type <node> program stmt stmt_list block device_decl action_stmt assign_stmt if_stmt loop_stmt event_stmt schedule_stmt log_stmt
%type <expr> expr

%%
program:
    stmt_list { root = ast_new_program($1); }
;

block:
      /* empty */ { $$ = NULL; }
    | stmt_list { $$ = $1; }
;

stmt_list:
      stmt { $$ = ast_append_stmt(NULL, $1); }
    | stmt_list stmt { $$ = ast_append_stmt($1, $2); }
;

stmt:
      device_decl ';' { $$ = $1; }
    | action_stmt ';' { $$ = $1; }
    | assign_stmt ';' { $$ = $1; }
    | log_stmt ';' { $$ = $1; }
    | if_stmt { $$ = $1; }
    | loop_stmt { $$ = $1; }
    | event_stmt { $$ = $1; }
    | schedule_stmt { $$ = $1; }
;

device_decl:
    DEVICE IDENT { $$ = ast_new_device_decl($2, yylineno); }
;

action_stmt:
      IDENT ON { $$ = ast_new_action($1, 1, yylineno); }
    | IDENT OFF { $$ = ast_new_action($1, 0, yylineno); }
;

assign_stmt:
    IDENT ASSIGN expr { $$ = ast_new_assign($1, $3, yylineno); }
;

log_stmt:
    LOG STRING { $$ = ast_new_log($2, yylineno); }
;

if_stmt:
    IF expr THEN block ELSE block END { $$ = ast_new_if($2, $4, $6, yylineno); }
  | IF expr THEN block END { $$ = ast_new_if($2, $4, NULL, yylineno); }
;

loop_stmt:
    LOOP NUMBER TIMES DO block END { $$ = ast_new_loop($2, $5, yylineno); }
;

event_stmt:
    ON expr THEN block END { $$ = ast_new_event($2, $4, yylineno); }
;

schedule_stmt:
    SCHEDULE AT TIME DO block END { $$ = ast_new_schedule(ast_parse_time($3), $5, yylineno); }
;

expr:
      expr PLUS expr { $$ = ast_new_binary_expr(OP_ADD, $1, $3); }
    | expr MINUS expr { $$ = ast_new_binary_expr(OP_SUB, $1, $3); }
    | expr MULT expr { $$ = ast_new_binary_expr(OP_MUL, $1, $3); }
    | expr DIV expr { $$ = ast_new_binary_expr(OP_DIV, $1, $3); }
    | expr GT expr { $$ = ast_new_binary_expr(OP_GT, $1, $3); }
    | expr LT expr { $$ = ast_new_binary_expr(OP_LT, $1, $3); }
    | expr GE expr { $$ = ast_new_binary_expr(OP_GE, $1, $3); }
    | expr LE expr { $$ = ast_new_binary_expr(OP_LE, $1, $3); }
    | expr EQ expr { $$ = ast_new_binary_expr(OP_EQ, $1, $3); }
    | expr NEQ expr { $$ = ast_new_binary_expr(OP_NEQ, $1, $3); }
    | '(' expr ')' { $$ = $2; }
    | NUMBER { $$ = ast_new_number_expr($1); }
    | IDENT { if (is_sensor_name($1)) { $$ = ast_new_variable_expr($1); } else { $$ = ast_new_variable_expr($1); } }
;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Syntax error at line %d: %s\n", yylineno, s);
}

int is_sensor_name(const char *name) {
    return strcmp(name, "temperature") == 0 || strcmp(name, "humidity") == 0 || strcmp(name, "light") == 0 || strcmp(name, "motion") == 0;
}
