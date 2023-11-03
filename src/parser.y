%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

// Flex functions and variables.
extern int yylex();
extern int yyparse(program_t*);
extern FILE *yyin;

program_t *ROOT = NULL;

void yyerror(program_t *root, const char *s);
void reconstruct_program(program_t*);
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
%left '-' '+' LTE GTE EQ
%token VAR
%token <ival> IDENT
%token SEMI
%token END_PROGRAM
%token <digitval> DIGITS
%token _BEGIN
%token _END
%token IF
%token THEN
%token WHILE
%token DO
%token ASSIGN
%token LTE
%token GTE
%token EQ
%type <stmtval> statements
%type <stmtval> statement
%type <exprval> expression
%type <decval> declaration
%type <decval> declarations
%parse-param {program_t *root}

%%
pl0:
    blocks END_PROGRAM
;

blocks:
    blocks block
|   block
;

block:
    statements {
        root->stmts = $statements;
    }
|   VAR declarations SEMI {
        root->decs = $declarations;
    }
;

declarations:
    declarations ',' declaration {
        var_dec_t **head = &(root->decs);
        var_dec_t **next_dec = &(root->decs);
        while (*next_dec != NULL) {
            next_dec = &(*next_dec)->next;
        }
        *next_dec = $3;
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
    IDENT {
        var_dec_t *v = malloc(sizeof(var_dec_t));
        v->var = $1;
        v->next = NULL;
        $$ = v;
    }
;

statements:
    statements SEMI statement {
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
|   _BEGIN statements _END {
        statement_t *begin_ = malloc(sizeof(statement_t));
        begin_->t = BEGIN_;
        begin_->begin_.body = $2;
        begin_->next = NULL;
        $$ = begin_;
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
    root->decs = NULL;
    root->stmts = NULL;
    yyparse(root);

    reconstruct_program(root);
}

void reconstruct_program(program_t *root)
{
    var_dec_t *list;
    list = root->decs;

    var_dec_t *tmp = NULL;
    while (list != NULL) {
        printf("var %s;\n", list->var);
        list = list->next;
    }

    statement_t *stmts = root->stmts;
    follow_stmts(stmts);
    printf(".\n");
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
    } else if (stmt->t == WHILE_) {
        printf("while ");
        descend_expr(stmt->while_.cond);
        printf("do\n");
        follow_stmts(stmt->while_.body);
    } else if (stmt->t == BEGIN_) {
        printf("begin\n");
        follow_stmts(stmt->begin_.body);
        printf("end\n");
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
    } else if (node->t == LTE_) {
        descend_expr(node->left);
        printf(" <= ");
        descend_expr(node->right);
    } else if (node->t == GTE_) {
        descend_expr(node->left);
        printf(" >= ");
        descend_expr(node->right);
    } else if (node->t == EQ_) {
        descend_expr(node->left);
        printf(" <= ");
        descend_expr(node->right);
    }
}

void yyerror(program_t *root, const char *s) {
    printf("Unknown line: %s\n", s);
}