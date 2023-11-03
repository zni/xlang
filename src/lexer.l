%{
#include <stdio.h>
#include "ast.h"
#include "parser.tab.h"
%}

DIGIT                   [0-9]
IDENT                   [a-z][a-z0-9]*
END_L                   ;
END_P                   [.]

%%

{DIGIT}+            { yylval.digitval = atoi(yytext); return DIGITS; }

(var|VAR)                   { return VAR; }

(begin|BEGIN)               { return _BEGIN; }

(end|END)                   { return _END; }

(if|IF)                     { return IF; }

(then|THEN)                 { return THEN; }

(while|WHILE)               { return WHILE; }

(do|DO)                     { return DO; }

:=                          { return ASSIGN; }

[+]                         { return '+'; }

-                           { return '-'; }

,                           { return ','; }

(<=)                        { return LTE; }

>=                          { return GTE; }

==                          { return EQ;  }

{IDENT}                     { yylval.ival = strdup(yytext); return IDENT; }

{END_L}                     { return SEMI; }

{END_P}                     { return END_PROGRAM; }

[ \t\r\n]+

%%
