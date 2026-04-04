%{
#include "parser.h"
#include "ast.h"
#include <stdlib.h>
#include <string.h>
void yyerror(const char *s);
%}

%%
[ \t\r]+                 ;
\xEF\xBB\xBF ; /* skip UTF-8 BOM */
"//".*                  ;
"/\*"([^*]|\*+[^*/])*\*+"/"    ;
"DEVICE"             return DEVICE;
"IF"               return IF;
"THEN"               return THEN;
"ELSE"               return ELSE;
"END"                return END;
"LOOP"               return LOOP;
"TIMES"              return TIMES;
"DO"                 return DO;
"ON"                 return ON;
"OFF"                return OFF;
"SCHEDULE"           return SCHEDULE;
"AT"                 return AT;
"LOG"                return LOG;
"=="                    return EQ;
"!="                    return NEQ;
">="                    return GE;
"<="                    return LE;
">"                     return GT;
"<"                     return LT;
"\+"                   return PLUS;
"-"                     return MINUS;
"\*"                   return MULT;
"/"                     return DIV;
"="                     return ASSIGN;
";"                     return ';';
"\("                   return '(';
"\)"                   return ')';
"\""([^\"]|\\.)*"\"" {
    yylval.str = strdup(yytext + 1);
    yylval.str[strlen(yylval.str) - 1] = '\0';
    return STRING;
}
[0-9]+":"[0-9]{2}       { yylval.str = strdup(yytext); return TIME; }
[0-9]+                   { yylval.num = atoi(yytext); return NUMBER; }
[a-zA-Z_][a-zA-Z0-9_]*    { yylval.str = strdup(yytext); return IDENT; }
\n                       { yylineno++; }
.                        { return yytext[0]; }
%%

int yywrap(void) {
    return 1;
}
