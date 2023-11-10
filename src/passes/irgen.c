#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "irgen.h"

unsigned int SYMBOL_INDEX = 0;

void convert_blocks_to_quads(block_t *block, quadblock_t *quads, env_t *env);
void convert_decs_to_quads(var_dec_t *dec, quadblock_t *quads, env_t *env);
void convert_stmts_to_quads(statement_t *stmt, quadblock_t *quads, env_t *env);
void convert_expr_to_stack(expression_t *node, expr_stack_t *stack);
char* convert_expr_stack_to_quads(expr_stack_t*, quadblock_t*, env_t *env);

char* new_symbol(env_t*, bool);
char* lookup_sym(env_t*, char*);

quadr_t* generate_binary_expr_quad(quad_op_t t, char *arg1, char *arg2, char *result);
quadr_t* generate_jump_target(char *label);
quadr_t* generate_cmp_zero(char *sym, env_t *env);
quadr_t* generate_jmp(quad_type_t jmp_type, char* jump_label);

quad_op_t expr_to_quadop(exprval_t);

void push(expr_stack_t *s, stack_item_t *item)
{
    if (s->next == NULL) {
        s->next = item;
        s->next->next = NULL;
    } else {
        item->next = s->next;
        s->next = item;
    }
}

stack_item_t* pop(expr_stack_t *s)
{
    if (s->next == NULL)
        return NULL;

    stack_item_t *tmp;
    tmp = s->next;
    s->next = tmp->next;
    tmp->next = NULL;

    return tmp;
}

expr_stack_t* new_stack()
{
    expr_stack_t *stack = malloc(sizeof(expr_stack_t));
    stack->pop = &pop;
    stack->push = &push;
    stack->next = NULL;
    return stack;
}

void append_block(quadblock_t *q, quadblock_t *qb)
{
    if (q->next == NULL) {
        q->next = qb;
        qb->next = NULL;
    } else {
        quadblock_t **tmp = &(q->next);
        while (*tmp != NULL)
            tmp = &(*tmp)->next;

        *tmp = qb;
    }
}

void append_line(quadblock_t *q, quadr_t *line)
{
    if (q->lines == NULL) {
        q->lines = line;
        line->next = NULL;
    } else {
        quadr_t **tmp = &(q->lines);
        while (*tmp != NULL)
            tmp = &(*tmp)->next;

        *tmp = line;
    }
}

quadblock_t* new_quadblock()
{
    quadblock_t *quadblock = malloc(sizeof(quadblock_t));
    quadblock->next = NULL;

    quadblock->append_block = &append_block;
    quadblock->append_line = &append_line;

    return quadblock;
}

void debug_stack(expr_stack_t *s)
{
    printf("%s\n", __FUNCTION__);
    stack_item_t *tmp = s->next;
    while (tmp != NULL) {
        switch (tmp->t) {
        case IDENT_:
            printf("%s\n", tmp->ident);
            break;
        case DIGITS_:
            printf("%d\n", tmp->digits);
            break;
        case ADD:
            printf("+\n");
            break;
        case SUB:
            printf("-\n");
            break;
        case MUL:
            printf("*\n");
            break;
        case DIV:
            printf("/\n");
            break;
        case LTE_:
            printf("<=\n");
            break;
        case GTE_:
            printf(">=\n");
            break;
        case EQ_:
            printf("==\n");
            break;
        }
        tmp = tmp->next;
    }
    printf("====\n");
}

void debug_quads(quadblock_t *q)
{
    quadblock_t *b = q->next;
    while (b != NULL) {
        printf("begin block\n");
        printf("===========\n");
        quadr_t *tmp = b->lines;
        while (tmp != NULL) {
            switch (tmp->op) {
            case Q_ADD:
                printf("\tADD\t%s,\t%s,\t%s\n", tmp->arg1.sym, tmp->arg2.sym, tmp->result.sym);
                break;
            case Q_SUB:
                printf("\tSUB\t%s,\t%s,\t%s\n", tmp->arg1.sym, tmp->arg2.sym, tmp->result.sym);
                break;
            case Q_MUL:
                printf("\tMUL\t%s,\t%s,\t%s\n", tmp->arg1.sym, tmp->arg2.sym, tmp->result.sym);
                break;
            case Q_DIV:
                printf("\tDIV\t%s,\t%s,\t%s\n", tmp->arg1.sym, tmp->arg2.sym, tmp->result.sym);
                break;
            case Q_GTE:
                printf("\tGTE\t%s,\t%s,\t%s\n", tmp->arg1.sym, tmp->arg2.sym, tmp->result.sym);
                break;
            case Q_LTE:
                printf("\tLTE\t%s,\t%s,\t%s\n", tmp->arg1.sym, tmp->arg2.sym, tmp->result.sym);
                break;
            case Q_EQ:
                printf("\tEQ\t%s,\t%s,\t%s\n", tmp->arg1.sym, tmp->arg2.sym, tmp->result.sym);
                break;
            case Q_CMP:
                printf("\tCMP\t");
                if (tmp->arg1.t == Q_CONSTANT) {
                    printf("%d,\t", tmp->arg1.constant);
                } else {
                    printf("%s,\t", tmp->arg1.sym);
                }
                if (tmp->arg2.t == Q_CONSTANT) {
                    printf("%d,\t", tmp->arg2.constant);
                } else {
                    printf("%s,\t", tmp->arg2.sym);
                }
                if (tmp->result.t == Q_CONSTANT) {
                    printf("%d\n", tmp->result.constant);
                } else {
                    printf("%s\n", tmp->result.sym);
                }
                break;
            case Q_NOP:
                printf("%s:\tNOP\n", tmp->label);
                break;
            case Q_GOTO:
                printf("\tGOTO\t%s\n", tmp->arg1.sym);
                break;
            case Q_CALL:
                printf("\tCALL\t%s\n", tmp->arg1.sym);
                break;
            case Q_STORE:
                if (tmp->arg1.t == Q_CONSTANT) {
                    printf("\tSTORE\t%d,\t\t%s\n", tmp->arg1.constant, tmp->result.sym);
                } else if (tmp->arg1.t == Q_SYMBOLIC) {
                    printf("\tSTORE\t%s,\t\t%s\n", tmp->arg1.sym, tmp->result.sym);
                }
                break;
            case Q_RETURN:
                printf("\tRETURN\n");
                break;
            }
            tmp = tmp->next;
        }
        printf("===========.\n\n");
        b = b->next;
    }
}

quadblock_t* convert_to_quads(program_t *program, env_t *env)
{
    quadblock_t *quadblocks = new_quadblock();
    convert_blocks_to_quads(program->blocks, quadblocks, env);
    debug_quads(quadblocks);
    return quadblocks;
}


void convert_blocks_to_quads(block_t *block, quadblock_t *quads, env_t *env)
{
    while (block != NULL) {
        if (block->t == PROCEDURE) {
            quadblock_t *procblock = new_quadblock();

            char *sub_label = new_symbol(env, false);
            env_data_t *data = malloc(sizeof(env_data_t));
            data->name = block->procedure.name;
            data->ir.orig = block->procedure.name;
            data->ir.sym = sub_label;
            data->ir.t = FUNCTION;
            add_entry(env, block->procedure.name, data);

            quadr_t *subroutine = malloc(sizeof(quadr_t));
            subroutine->t = Q_NOP;
            subroutine->op = Q_NOP;
            subroutine->label = sub_label;
            subroutine->arg1.t = Q_NONE;
            subroutine->arg2.t = Q_NONE;
            subroutine->result.t = Q_NONE;
            procblock->append_line(procblock, subroutine);

            block_t *ctx = block->procedure.context;
            while (ctx != NULL) {
                if (ctx->t == STATEMENT) {
                    convert_stmts_to_quads(ctx->stmts, procblock, env);
                }
                ctx = ctx->next;
            }

            quadr_t *rts = malloc(sizeof(quadr_t));
            rts->t = Q_PROCEDURE;
            rts->op = Q_RETURN;
            rts->label = NULL;
            rts->arg1.t = Q_NONE;
            rts->arg2.t = Q_NONE;
            rts->result.t = Q_NONE;
            procblock->append_line(procblock, rts);
            quads->append_block(quads, procblock);
        } else if (block->t == DECLARATION) {
            convert_decs_to_quads(block->decs, quads, env);
        } else if (block->t == STATEMENT) {
            quadblock_t *stmtblock = new_quadblock();
            convert_stmts_to_quads(block->stmts, stmtblock, env);
            quads->append_block(quads, stmtblock);
        }
        block = block->next;
    }
}

void convert_decs_to_quads(var_dec_t *dec, quadblock_t *quads, env_t *env)
{
    while (dec != NULL) {
        env_data_t *data = malloc(sizeof(env_data_t));
        data->name = dec->var;
        data->ir.orig = dec->var;
        data->ir.sym = new_symbol(env, false);
        data->ir.t = VARIABLE_NAME;
        add_entry(env, dec->var, data);

        dec = dec->next;
    }
}

void convert_stmts_to_quads(statement_t *stmt, quadblock_t *quads, env_t *env)
{
    if (stmt == NULL) {
        return;
    } else if (stmt->t == ASSIGN_) {
        char *sym = lookup_sym(env, stmt->assign.var);
        expr_stack_t *stack = new_stack();
        convert_expr_to_stack(stmt->assign.value, stack);
        //debug_stack(stack);
        char *result_sym = convert_expr_stack_to_quads(stack, quads, env);
        quadr_t *assign = malloc(sizeof(quadr_t));
        assign->t = Q_COPY;
        assign->label = NULL;
        assign->op = Q_STORE;
        assign->arg1.t = Q_SYMBOLIC;
        assign->arg1.sym = result_sym;
        assign->arg2.t = Q_NONE;
        assign->result.t = Q_SYMBOLIC;
        assign->result.sym = sym;
        quads->append_line(quads, assign);
    } else if (stmt->t == IF_) {
        expr_stack_t *stack = new_stack();
        convert_expr_to_stack(stmt->if_.cond, stack);
        char *result_sym = convert_expr_stack_to_quads(stack, quads, env);

        quadr_t *cmp = generate_cmp_zero(result_sym, env);
        quads->append_line(quads, cmp);

        char *jump_label = new_symbol(env, false);
        quadr_t *branch = generate_jmp(Q_COND_JMP, jump_label);
        quads->append_line(quads, branch);

        convert_stmts_to_quads(stmt->if_.body, quads, env);

        quadr_t *jump_target = generate_jump_target(jump_label);
        quads->append_line(quads, jump_target);

    } else if (stmt->t == WHILE_) {
        char *backward_label = new_symbol(env, false);
        quadr_t *backward_target = generate_jump_target(backward_label);
        quads->append_line(quads, backward_target);

        expr_stack_t *stack = new_stack();
        convert_expr_to_stack(stmt->while_.cond, stack);
        char *result_sym = convert_expr_stack_to_quads(stack, quads, env);

        quadr_t *cmp = generate_cmp_zero(result_sym, env);
        quads->append_line(quads, cmp);

        char *forward_label = new_symbol(env, false);
        quadr_t *forward_jmp = generate_jmp(Q_COND_JMP, forward_label);
        quads->append_line(quads, forward_jmp);

        convert_stmts_to_quads(stmt->while_.body, quads, env);

        quadr_t *backward_jmp = generate_jmp(Q_UNCOND_JMP, backward_label);
        quads->append_line(quads, backward_jmp);

        quadr_t *forward_target = generate_jump_target(forward_label);
        quads->append_line(quads, forward_target);

    } else if (stmt->t == BEGIN_) {
        convert_stmts_to_quads(stmt->begin_.body, quads, env);
    } else if (stmt->t == CALL_) {
        quadr_t *call = malloc(sizeof(quadr_t));
        call->t = Q_PROCEDURE;
        call->op = Q_CALL;
        call->label = NULL;
        call->arg1.t = Q_SYMBOLIC;
        call->arg1.sym = lookup_sym(env, stmt->call_.var);
        call->arg2.t = Q_NONE;
        call->result.t = Q_NONE;
        quads->append_line(quads, call);

    }
    convert_stmts_to_quads(stmt->next, quads, env);
}

char* resolve_op(stack_item_t *op, quadblock_t *quads, env_t *env)
{
    char *sym = new_symbol(env, true);
    if (op->t == DIGITS_) {
        quadr_t *q = malloc(sizeof(quadr_t));
        q->t = Q_COPY;
        q->label = NULL;
        q->op = Q_STORE;
        q->arg1.t = Q_CONSTANT;
        q->arg1.constant = op->digits;
        q->arg2.t = Q_NONE;
        q->result.t = Q_SYMBOLIC;
        q->result.sym = sym;
        quads->append_line(quads, q);
    } else if (op->t == IDENT_) {
        char *varsym = lookup_sym(env, op->ident);
        quadr_t *q = malloc(sizeof(quadr_t));
        q->t = Q_COPY;
        q->label = NULL;
        q->op = Q_STORE;
        q->arg1.t = Q_VARIABLE;
        q->arg1.sym = varsym;
        q->arg2.t = Q_NONE;
        q->result.t = Q_SYMBOLIC;
        q->result.sym = sym;
        quads->append_line(quads, q);
    }

    return sym;
}

void resolve_destination(stack_item_t *expr, expr_stack_t *stack, expr_stack_t *backpatch, quadblock_t *quads, env_t *env)
{
    if (expr->t == IDENT_) {
        char *varsym = lookup_sym(env, expr->ident);
        quadr_t *q = malloc(sizeof(quadr_t));
        q->t = Q_COPY;
        q->label = NULL;
        q->op = Q_STORE;
        q->arg1.t = Q_VARIABLE;
        q->arg1.sym = varsym;
        q->arg2.t = Q_NONE;
        q->result.t = Q_SYMBOLIC;
        q->result.sym = new_symbol(env, true);
        quads->append_line(quads, q);
    } else if (expr->t == DIGITS_) {
        quadr_t *q = malloc(sizeof(quadr_t));
        q->t = Q_COPY;
        q->label = NULL;
        q->op = Q_STORE;
        q->arg1.t = Q_CONSTANT;
        q->arg1.constant = expr->digits;
        q->arg2.t = Q_NONE;
        q->result.t = Q_SYMBOLIC;
        q->result.sym = new_symbol(env, true);
        quads->append_line(quads, q);
    } else {
        quad_op_t quad_type = expr_to_quadop(expr->t);
        stack_item_t *op1 = stack->pop(stack);
        stack_item_t *op2 = stack->pop(stack);
        char *result_sym = new_symbol(env, true);
        if (op2->t != IDENT_ && op2->t != DIGITS_) {
            resolve_destination(op2, stack, backpatch, quads, env);
            quadr_t *q = generate_binary_expr_quad(
                quad_type,
                resolve_op(op1, quads, env),
                (backpatch->pop(backpatch))->ident,
                result_sym
            );
            quads->append_line(quads, q);
        } else {
            quadr_t *q = generate_binary_expr_quad(
                quad_type,
                resolve_op(op1, quads, env),
                resolve_op(op2, quads, env),
                result_sym
            );
            quads->append_line(quads, q);
        }
        stack_item_t *result = malloc(sizeof(stack_item_t));
        result->t = IDENT_;
        result->ident = result_sym;
        result->next = NULL;
        backpatch->push(backpatch, result);
    }
}


char* convert_expr_stack_to_quads(expr_stack_t *stack, quadblock_t *quads, env_t *env)
{
    char *result_sym;
    while (stack->next != NULL) {
        stack_item_t *top = stack->pop(stack);
        if (top->t == IDENT_) {
            char *varsym = lookup_sym(env, top->ident);
            result_sym = new_symbol(env, true);
            quadr_t *q = malloc(sizeof(quadr_t));
            q->t = Q_COPY;
            q->label = NULL;
            q->op = Q_STORE;
            q->arg1.t = Q_VARIABLE;
            q->arg1.sym = varsym;
            q->arg2.t = Q_NONE;
            q->result.t = Q_SYMBOLIC;
            q->result.sym = result_sym;
            quads->append_line(quads, q);
        } else if (top->t == DIGITS_) {
            result_sym = new_symbol(env, true);
            quadr_t *q = malloc(sizeof(quadr_t));
            q->t = Q_COPY;
            q->label = NULL;
            q->op = Q_STORE;
            q->arg1.t = Q_CONSTANT;
            q->arg1.constant = top->digits;
            q->arg2.t = Q_NONE;
            q->result.t = Q_SYMBOLIC;
            q->result.sym = result_sym;
            quads->append_line(quads, q);
        } else {
            quad_op_t quad_type = expr_to_quadop(top->t);
            stack_item_t *op1 = stack->pop(stack);
            stack_item_t *op2 = stack->pop(stack);
            if (op2->t != IDENT_ && op2->t != DIGITS_) {
                expr_stack_t *backpatch = new_stack();
                resolve_destination(op2, stack, backpatch, quads, env);
                result_sym = new_symbol(env, true);
                quadr_t *q = generate_binary_expr_quad(
                    quad_type,
                    resolve_op(op1, quads, env),
                    (backpatch->pop(backpatch))->ident,
                    result_sym
                );
                quads->append_line(quads, q);
            } else {
                result_sym = new_symbol(env, true);
                quadr_t *q = generate_binary_expr_quad(
                    quad_type,
                    resolve_op(op1, quads, env),
                    resolve_op(op2, quads, env),
                    result_sym
                );
                quads->append_line(quads, q);
            }
        }
    }

    return result_sym;
}

void convert_expr_to_stack(expression_t *node, expr_stack_t *s)
{
    if (node == NULL) {
        return;
    }else if (node->t == IDENT_) {
        stack_item_t *item = malloc(sizeof(stack_item_t));
        item->t = node->t;
        item->ident = node->ident;
        s->push(s, item);
        return;
    } else if (node->t == DIGITS_) {
        stack_item_t *item = malloc(sizeof(stack_item_t));
        item->t = node->t;
        item->digits = node->digits;
        s->push(s, item);
        return;
    } else {
        convert_expr_to_stack(node->left, s);
        convert_expr_to_stack(node->right, s);
        stack_item_t *item = malloc(sizeof(stack_item_t));
        item->t = node->t;
        s->push(s, item);
    }
}

char* new_symbol(env_t *env, bool update_env)
{
    char *sym = malloc(sizeof(char) * 8);
    sprintf(sym, "s%03u", SYMBOL_INDEX);
    SYMBOL_INDEX++;

    if (!update_env)
        return sym;

    env_data_t *data = malloc(sizeof(env_data_t));
    data->name = sym;
    data->ir.orig = sym;
    data->ir.sym = sym;
    data->ir.t = SYMBOLIC;
    add_entry(env, sym, data);

    return sym;
}

quadr_t* generate_binary_expr_quad(quad_op_t op, char *arg1, char *arg2, char *result)
{
    quadr_t *bin = malloc(sizeof(quadr_t));
    bin->t = Q_BINARY;
    bin->label = NULL;
    bin->op = op;
    bin->arg1.t = Q_SYMBOLIC;
    bin->arg1.sym = arg1;
    bin->arg2.t = Q_SYMBOLIC;
    bin->arg2.sym = arg2;
    bin->result.t = Q_SYMBOLIC;
    bin->result.sym = result;

    return bin;
}

quadr_t* generate_jump_target(char *label)
{
    quadr_t *jump_target = malloc(sizeof(quadr_t));
    jump_target->t = Q_NOP;
    jump_target->label = label;
    jump_target->op = Q_NOP;
    jump_target->arg1.t = Q_NONE;
    jump_target->arg2.t = Q_NONE;
    jump_target->result.t = Q_NONE;

    return jump_target;
}

quadr_t* generate_cmp_zero(char *sym, env_t *env)
{
    quadr_t *cmp = malloc(sizeof(quadr_t));
    cmp->t = Q_BINARY;
    cmp->label = NULL;
    cmp->op = Q_CMP;
    cmp->arg1.t = Q_CONSTANT;
    cmp->arg1.constant = 0;
    cmp->arg2.t = Q_SYMBOLIC;
    cmp->arg2.sym = sym;
    cmp->result.t = Q_SYMBOLIC;
    cmp->result.sym = new_symbol(env, true);
    return cmp;
}

quadr_t* generate_jmp(quad_type_t jmp_type, char *jump_label)
{
    quadr_t *branch = malloc(sizeof(quadr_t));
    branch->t = jmp_type;
    branch->label = NULL;
    branch->op = Q_GOTO;
    branch->arg1.t = Q_SYMBOLIC;
    branch->arg1.sym = jump_label;
    branch->arg2.t = Q_NONE;
    branch->result.t = Q_NONE;
    return branch;
}

char* lookup_sym(env_t *env, char *key)
{
    env_data_t *var = lookup_entry(env, key);
    if (var == NULL) {
        fprintf(stderr, "Failed to lookup symbol... Bailing out.\n");
        exit(1);
    }

    return var->ir.sym;
}

quad_op_t expr_to_quadop(exprval_t t)
{
    switch (t) {
    case ADD:
        return Q_ADD;
    case SUB:
        return Q_SUB;
    case MUL:
        return Q_MUL;
    case DIV:
        return Q_DIV;
    case LTE_:
        return Q_LTE;
    case GTE_:
        return Q_GTE;
    case EQ_:
        return Q_EQ;

    case IDENT_:
    case DIGITS_:
    default:
        return -1;
    }
}
