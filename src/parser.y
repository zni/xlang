%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

// Flex functions and variables.
extern int yylex();
extern int yyparse();
extern FILE *yyin;

// TODO Try to eliminate HEAD.
// var_dec_t *HEAD = NULL;
program_t *ROOT = NULL;

void yyerror(program_t *root, const char *s);
void follow_stmts(statement_t*);
void descend_expr(expression_t*);
%}
%union {
    int digitval;
    char *ival;
    statement_t *stmtval;
    expression_t *exprval;
    var_dec_t *decval;
    program_t *progval;
}
%left '-' '+'
%token VAR
%token <ival> IDENT
%token SEMI
%token END_PROGRAM
%token <digitval> DIGITS
%token _BEGIN
%token _END
%token IF
%token THEN
%token ASSIGN
%type <stmtval> statements
%type <stmtval> statement
%type <exprval> expression
%type <decval> declaration
%type <decval> declarations
%parse-param {program_t *root}

%%
pl0:
    block END_PROGRAM
;

block:
   declarations _BEGIN statements _END {
        root->decs = $declarations;
        root->stmts = $statements;
    }
;

declarations:
    declarations declaration {
        var_dec_t **head = &(root->decs);
        var_dec_t **next_dec = &(root->decs);
        while (*next_dec != NULL) {
            next_dec = &(*next_dec)->next;
        }
        *next_dec = $2;
        $$ = *head;
    }
|   declaration {
        var_dec_t **head = &(root->decs);
        var_dec_t **next_dec = &(root->decs);
        while (*next_dec != NULL) {
            next_dec = &(*next_dec)->next;
        }
        *next_dec = $1;
        $$ = *head; 
    }
;

declaration:
    VAR IDENT SEMI {
        var_dec_t *v = malloc(sizeof(var_dec_t));
        v->var = $2;
        v->next = NULL;
        $$ = v;
    }
;

statements:
    statements SEMI statement {
        statement_t **head = &(root->stmts);
        statement_t **next_stmt = &(root->stmts);
        while (*next_stmt != NULL) {
            next_stmt = &(*next_stmt)->next;
        }
        *next_stmt = $3;
        $$ = *head;
    }
|   statement{
        statement_t **head = &(root->stmts);
        statement_t **next_stmt = &(root->stmts);
        while (*next_stmt != NULL) {
            next_stmt = &(*next_stmt)->next;
        }
        *next_stmt = $1;
        $$ = *head;
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
    root->decs = NULL;
    root->stmts = NULL;
    yyparse(root);

    if (root == NULL) {
        printf("root is NULL\n");
        return 1;
    }
    
    var_dec_t *list;
    list = root->decs;

    var_dec_t *tmp = NULL;
    while (list != NULL) {
        printf("\tvar %s;\n", list->var);
        list = list->next;
    }

    statement_t *stmts = root->stmts;
    follow_stmts(stmts);
}

void follow_stmts(statement_t *stmt)
{
    if (stmt == NULL) {
        return;
    } else if (stmt->t == ASSIGN_) {
        printf("%s := ", stmt->assign.var);
        descend_expr(stmt->assign.value);
        putchar('\n');
    } else if (stmt->t == IF_) {
        printf("if ");
        descend_expr(stmt->if_.cond);
        printf("then\n");
        follow_stmts(stmt->if_.body);
    }
    follow_stmts(stmt->next);
}

void descend_expr(expression_t *node) 
{
    if (node == NULL) {
        return;
    }else if (node->t == IDENT_) {
        printf("%s ", node->ident);
        return;
    } else if (node->t == DIGITS_) {
        printf("%d ", node->digits);
        return;
    } else if (node->t == ADD) {
        descend_expr(node->left);
        printf(" + ");
        descend_expr(node->right);
    } else if (node->t == SUB) {
        descend_expr(node->left);
        printf(" - ");
        descend_expr(node->right);
    }
}

void yyerror(program_t *root, const char *s) {
    printf("Unknown line: %s\n", s);
}