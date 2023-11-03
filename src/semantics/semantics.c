#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include "semantics.h"
#include "env.h"

void follow_blocks(block_t *block, env_t *env);
void follow_decs(var_dec_t *dec, env_t *env);
void follow_stmts(statement_t *stmt, env_t *env);
void descend_expr(expression_t *node);

void semantic_analysis(program_t *root)
{
    env_t env;
    init_env(&env);

    block_t *blocks;
    blocks = root->blocks;
    follow_blocks(blocks, &env);
}

void follow_blocks(block_t *block, env_t *env)
{
    while (block != NULL) {
        if (block->t == PROCEDURE) {
            env_data_t *data = malloc(sizeof(env_data_t));
            data->name = block->procedure.name;
            add_entry(env, block->procedure.name, data);
            follow_blocks(block->procedure.context, env);
        } else if (block->t == DECLARATION) {
            follow_decs(block->decs, env);
        } else if (block->t == STATEMENT) {
            follow_stmts(block->stmts, env);
        }
        block = block->next;
    }
}

void follow_decs(var_dec_t *dec, env_t *env)
{
    while (dec != NULL) {
        env_data_t *data = malloc(sizeof(env_data_t));
        data->name = dec->var;
        add_entry(env, dec->var, data);
        dec = dec->next;
    }
}

void follow_stmts(statement_t *stmt, env_t *env)
{
    if (stmt == NULL) {
        return;
    } else if (stmt->t == ASSIGN_) {
        if (lookup_entry(env, stmt->assign.var) == NULL) {
            printf("Variable %s used before declaration.\n", stmt->assign.var);
        }
        descend_expr(stmt->assign.value);
    } else if (stmt->t == IF_) {
        descend_expr(stmt->if_.cond);
        follow_stmts(stmt->if_.body, env);
    } else if (stmt->t == WHILE_) {
        descend_expr(stmt->while_.cond);
        follow_stmts(stmt->while_.body, env);
    } else if (stmt->t == BEGIN_) {
        follow_stmts(stmt->begin_.body, env);
    } else if (stmt->t == CALL_) {
        if (lookup_entry(env, stmt->call_.var) == NULL) {
            printf("Function %s called before declaration.\n", stmt->call_.var);
        }
    }
    follow_stmts(stmt->next, env);
}

void descend_expr(expression_t *node)
{
    if (node == NULL) {
        return;
    }else if (node->t == IDENT_) {
        return;
    } else if (node->t == DIGITS_) {
        return;
    } else if (node->t == ADD) {
        descend_expr(node->left);
        descend_expr(node->right);
    } else if (node->t == SUB) {
        descend_expr(node->left);
        descend_expr(node->right);
    } else if (node->t == MUL) {
        descend_expr(node->left);
        descend_expr(node->right);
    } else if (node->t == DIV) {
        descend_expr(node->left);
        descend_expr(node->right);
    } else if (node->t == LTE_) {
        descend_expr(node->left);
        descend_expr(node->right);
    } else if (node->t == GTE_) {
        descend_expr(node->left);
        descend_expr(node->right);
    } else if (node->t == EQ_) {
        descend_expr(node->left);
        descend_expr(node->right);
    }
}