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

var                 { return VAR; }

begin               { return _BEGIN; }

end                 { return _END; }

if                  { return IF; }

then                { return THEN; }

:=                  { return ASSIGN; }

[+]                 { return '+'; }

-                   { return '-'; }

{IDENT}             { yylval.ival = strdup(yytext); return IDENT; }

{END_L}             { return SEMI; }

{END_P}             { return END_PROGRAM; }

[ \t\r\n]+

%%
