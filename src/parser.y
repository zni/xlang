%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "semantics.h"

// Flex functions and variables.
extern int yylex();
extern int yyparse(program_t*);
extern FILE *yyin;

program_t *ROOT = NULL;

void yyerror(program_t *root, const char *s);
%}
%union {
    int digitval;
    char *ival;
    block_t *blockval;
    statement_t *stmtval;
    expression_t *exprval;
    var_dec_t *decval;
    program_t *progval;
}
%left '-' '+' '*' '/' LTE GTE EQ
%token VAR
%token <ival> IDENT
%token END_PROGRAM
%token <digitval> DIGITS
%token PROC
%token _BEGIN
%token _END
%token IF
%token THEN
%token WHILE
%token DO
%token ASSIGN
%token CALL
%token LTE
%token GTE
%token EQ
%type <blockval> blocks
%type <blockval> block
%type <stmtval> sequence_statements
%type <stmtval> statement
%type <exprval> expression
%type <decval> declaration
%type <decval> declarations
%parse-param {program_t *root}

%%
pl0:
    blocks END_PROGRAM {
        root->blocks = $1;
    }
;

blocks:
    blocks block {
        block_t **head = &($1);
        block_t **next_blk = &($1);
        while (*next_blk != NULL) {
            next_blk = &(*next_blk)->next;
        }
        *next_blk = $2;
        $$ = *head;
    }
|   block {
        $$ = $1;
    }
;

block:
    statement {
        block_t *block = malloc(sizeof(block_t));
        block->t = STATEMENT;
        block->stmts = $1;
        $$ = block;
    }
|   VAR declarations ';' {
        block_t *block = malloc(sizeof(block_t));
        block->t = DECLARATION;
        block->decs = $2;
        $$ = block;
    }
|   PROC IDENT ';' blocks ';' {
        block_t *block = malloc(sizeof(block_t));
        block->t = PROCEDURE;
        block->procedure.name = $2;
        block->procedure.context = $4;
        $$ = block;
    }
;

declarations:
    declarations ',' declaration {
        var_dec_t **head = &($1);
        var_dec_t **next_dec = &($1);
        while (*next_dec != NULL) {
            next_dec = &(*next_dec)->next;
        }
        *next_dec = $3;
        $$ = *head;
    }
|   declaration {
        $$ = $1;
    }
;

declaration:
    IDENT {
        var_dec_t *v = malloc(sizeof(var_dec_t));
        v->var = $1;
        v->next = NULL;
        $$ = v;
    }
;

statement:
    IF expression THEN statement { 
        statement_t *if_ = malloc(sizeof(statement_t));
        if_->t = IF_;
        if_->if_.cond = $2;
        if_->if_.body = $4;
        if_->next = NULL;
        $$ = if_;
    }
|   IDENT ASSIGN expression { 
        statement_t *assign = malloc(sizeof(statement_t));
        assign->t = ASSIGN_;
        assign->assign.var = $1;
        assign->assign.value = $3;
        assign->next = NULL;
        $$ = assign;
    }
|   WHILE expression DO statement {
        statement_t *while_ = malloc(sizeof(statement_t));
        while_->t = WHILE_;
        while_->while_.cond = $2;
        while_->while_.body = $4;
        while_->next = NULL;
        $$ = while_;
    }
|   _BEGIN sequence_statements _END {
        statement_t *begin_ = malloc(sizeof(statement_t));
        begin_->t = BEGIN_;
        begin_->begin_.body = $2;
        begin_->next = NULL;
        $$ = begin_;
    }
|   CALL IDENT {
        statement_t *call = malloc(sizeof(statement_t));
        call->t = CALL_;
        call->call_.var = $2;
        call->next = NULL;
        $$ = call;
    }
;

sequence_statements:
    sequence_statements ';' statement {
        statement_t **head = &($1);
        statement_t **next_stmt = &($1);
        while (*next_stmt != NULL) {
            next_stmt = &(*next_stmt)->next;
        }
        *next_stmt = $3;
        $$ = *head;
    }
|   statement {
        $$ = $1;
    }
;

expression:
    DIGITS  {
        expression_t *expr = malloc(sizeof(expression_t));
        expr->t = DIGITS_;
        expr->digits = $1;
        expr->left = NULL;
        expr->right = NULL;
        $$ = expr;
    }
|   IDENT {
        expression_t *expr = malloc(sizeof(expression_t));
        expr->t = IDENT_;
        expr->ident = $1;
        expr->left = NULL;
        expr->right = NULL;
        $$ = expr;
    }
|   expression '+' expression {
        expression_t *expr = malloc(sizeof(expression_t));
        expr->t = ADD;
        expr->left = $1;
        expr->right = $3;
        $$ = expr;
    }
|   expression '-' expression {
        expression_t *expr = malloc(sizeof(expression_t));
        expr->t = SUB;
        expr->left = $1;
        expr->right = $3;
        $$ = expr;
    }
|   expression '*' expression {
        expression_t *expr = malloc(sizeof(expression_t));
        expr->t = MUL;
        expr->left = $1;
        expr->right = $3;
        $$ = expr;
    }
|   expression '/' expression {
        expression_t *expr = malloc(sizeof(expression_t));
        expr->t = DIV;
        expr->left = $1;
        expr->right = $3;
        $$ = expr;
    }
|   expression LTE expression {
        expression_t *expr = malloc(sizeof(expression_t));
        expr->t = LTE_;
        expr->left = $1;
        expr->right = $3;
        $$ = expr;
    }
|   expression GTE expression {
        expression_t *expr = malloc(sizeof(expression_t));
        expr->t = GTE_;
        expr->left = $1;
        expr->right = $3;
        $$ = expr;
    }
|   expression EQ expression {
        expression_t *expr = malloc(sizeof(expression_t));
        expr->t = EQ_;
        expr->left = $1;
        expr->right = $3;
        $$ = expr;
    }
;
%%

int main(int argc, char **argv)
{
    --argc;
    ++argv;
    if (argc > 0) {
        yyin = fopen(argv[0], "r");
    } else {
        yyin = stdin;
    }
    program_t *root = malloc(sizeof(program_t));
    root->blocks = NULL;
    yyparse(root);


    semantic_analysis(root);
}

void yyerror(program_t *root, const char *s) {
    printf("Unknown line: %s\n", s);
}