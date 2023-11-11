#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "irgen.h"

unsigned int SYMBOL_INDEX = 0;

quadblock_t* convert_blocks_to_quads(block_t *block, env_t *env);
void convert_decs_to_quads(var_dec_t *dec, quadblock_t *quads, env_t *env);
void convert_stmts_to_quads(statement_t *stmt, quadblock_t *quads, env_t *env);
void convert_expr_to_stack(expression_t *node, expr_stack_t *stack);
char* convert_expr_stack_to_quads(expr_stack_t*, quadblock_t*, env_t *env);

char* new_symbol(env_t*, bool);
char* lookup_sym(env_t*, char*);

quadr_t* generate_word_storage(char *name);
quadr_t* generate_binary_expr_quad(quad_op_t t, char *arg1, char *arg2, char *result, env_t *env);
quadr_t* generate_jump_target(char *label);
quadr_t* generate_cmp_zero(char *sym, env_t *env);
quadr_t* generate_jmp(quad_type_t jmp_type, quad_op_t jmp_op, char* jump_label);

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
    quadblock_t *b = q;
    while (b != NULL) {
        printf("begin block\n");
        printf("===========\n");
        printf("type: ");
        if (b->t == QB_DATA) {
            printf("QB_DATA\n");
        } else {
            printf("QB_CODE\n");
        }
        printf("===========\n");

        quadr_t *tmp = b->lines;
        while (tmp != NULL) {
            switch (tmp->op) {
            case Q_ADD:
                printf("\tADD\t");
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
            case Q_SUB:
                printf("\tSUB\t");
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
            case Q_MUL:
                printf("\tMUL\t");
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
            case Q_DIV:
                printf("\tDIV\t");
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
                if (tmp->label != NULL)
                    printf("%s:\tNOP\n", tmp->label);
                else
                    printf("\tNOP\n");
                break;
            case Q_GOTO:
                printf("\tGOTO\t%s\n", tmp->arg1.sym);
                break;
            case Q_CALL:
                printf("\tCALL\t%s\n", tmp->arg1.sym);
                break;
            case Q_STORE:
                printf("\tSTORE\t");
                if (tmp->arg1.t == Q_CONSTANT) {
                    printf("%d,\t", tmp->arg1.constant);
                } else {
                    printf("%s,\t", tmp->arg1.sym);
                }
                if (tmp->result.t == Q_CONSTANT) {
                    printf("%d\n", tmp->result.constant);
                } else {
                    printf("%s\n", tmp->result.sym);
                }
                break;
            case Q_RETURN:
                printf("\tRETURN\n");
                break;
            case Q_WORD:
                printf("WORD\t%s\n", tmp->label);
            }
            tmp = tmp->next;
        }
        printf("===========.\n\n");
        b = b->next;
    }
}

quadblock_t* convert_to_quads(program_t *program, env_t *env)
{
    quadblock_t *quadblocks = NULL;
    block_t *block = program->blocks;
    while (block != NULL) {
        if (quadblocks == NULL)
            quadblocks = convert_blocks_to_quads(program->blocks, env);
        else
            quadblocks->append_block(quadblocks, convert_blocks_to_quads(block, env));

        block = block->next;
    }
    //debug_quads(quadblocks);
    return quadblocks;
}


quadblock_t* convert_blocks_to_quads(block_t *block, env_t *env)
{
    if (block->t == PROCEDURE) {
        quadblock_t *procblock = new_quadblock();
        procblock->t = QB_CODE;

        char *sub_label = new_symbol(env, false);
        env_data_t *data = malloc(sizeof(env_data_t));
        data->name = block->procedure.name;
        data->ir.orig = block->procedure.name;
        data->ir.sym = sub_label;
        data->ir.t = ENV_FUNCTION;
        add_entry(env, block->procedure.name, data);

        quadr_t *subroutine = malloc(sizeof(quadr_t));
        subroutine->t = QT_NOP;
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
        rts->t = QT_PROCEDURE;
        rts->op = Q_RETURN;
        rts->label = NULL;
        rts->arg1.t = Q_NONE;
        rts->arg2.t = Q_NONE;
        rts->result.t = Q_NONE;
        procblock->append_line(procblock, rts);
        return procblock;
    } else if (block->t == DECLARATION) {
        quadblock_t *storage_block = new_quadblock();
        storage_block->t = QB_DATA;
        convert_decs_to_quads(block->decs, storage_block, env);
        return storage_block;
    } else {
        quadblock_t *stmtblock = new_quadblock();
        stmtblock->t = QB_CODE;
        convert_stmts_to_quads(block->stmts, stmtblock, env);
        append_line(stmtblock, generate_jump_target(NULL));
        return stmtblock;
    }
}

void convert_decs_to_quads(var_dec_t *dec, quadblock_t *quads, env_t *env)
{
    while (dec != NULL) {
        env_data_t *data = malloc(sizeof(env_data_t));
        char *sym = new_symbol(env, false);
        data->name = dec->var;
        data->ir.orig = dec->var;
        data->ir.sym = sym;
        data->ir.t = ENV_VARIABLE;
        add_entry(env, dec->var, data);
        quads->append_line(quads, generate_word_storage(sym));

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
        char *result_sym = convert_expr_stack_to_quads(stack, quads, env);
        quadr_t *assign = malloc(sizeof(quadr_t));
        assign->t = QT_COPY;
        assign->label = NULL;
        assign->op = Q_STORE;
        assign->arg1.t = Q_SYMBOLIC;
        assign->arg1.sym = result_sym;
        assign->arg1.register_priority = 0;
        assign->arg2.t = Q_NONE;
        assign->result.t = Q_VARIABLE;
        assign->result.sym = sym;
        quads->append_line(quads, assign);
    } else if (stmt->t == IF_) {
        expr_stack_t *stack = new_stack();
        convert_expr_to_stack(stmt->if_.cond, stack);
        char *result_sym = convert_expr_stack_to_quads(stack, quads, env);

        quadr_t *cmp = generate_cmp_zero(result_sym, env);
        quads->append_line(quads, cmp);

        char *jump_label = new_symbol(env, false);
        quadr_t *branch = generate_jmp(QT_COND_JMP, Q_BEQ, jump_label);
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
        quadr_t *forward_jmp = generate_jmp(QT_COND_JMP, Q_BEQ, forward_label);
        quads->append_line(quads, forward_jmp);

        convert_stmts_to_quads(stmt->while_.body, quads, env);

        quadr_t *backward_jmp = generate_jmp(QT_UNCOND_JMP, Q_BR, backward_label);
        quads->append_line(quads, backward_jmp);

        quadr_t *forward_target = generate_jump_target(forward_label);
        quads->append_line(quads, forward_target);

    } else if (stmt->t == BEGIN_) {
        convert_stmts_to_quads(stmt->begin_.body, quads, env);
    } else if (stmt->t == CALL_) {
        quadr_t *call = malloc(sizeof(quadr_t));
        call->t = QT_PROCEDURE;
        call->op = Q_CALL;
        call->label = NULL;
        call->arg1.t = Q_VARIABLE;
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
        q->t = QT_COPY;
        q->label = NULL;
        q->op = Q_STORE;
        q->arg1.t = Q_CONSTANT;
        q->arg1.constant = op->digits;
        q->arg2.t = Q_NONE;
        q->result.t = Q_SYMBOLIC;
        q->result.register_priority = 0;
        q->result.sym = sym;
        quads->append_line(quads, q);
    } else if (op->t == IDENT_) {
        char *varsym = lookup_sym(env, op->ident);
        quadr_t *q = malloc(sizeof(quadr_t));
        q->t = QT_COPY;
        q->label = NULL;
        q->op = Q_STORE;
        q->arg1.t = Q_VARIABLE;
        q->arg1.sym = varsym;
        q->arg2.t = Q_NONE;
        q->result.t = Q_SYMBOLIC;
        q->result.register_priority = 0;
        q->result.sym = sym;
        quads->append_line(quads, q);
    }

    return sym;
}

char* resolve_destination(expr_stack_t *stack, expr_stack_t *backpatch, quadblock_t *quads, env_t *env)
{
    char *result_sym = NULL;
    stack_item_t *expr = stack->pop(stack);
    if (expr->t == IDENT_) {
        char *varsym = lookup_sym(env, expr->ident);
        quadr_t *q = malloc(sizeof(quadr_t));
        q->t = QT_COPY;
        q->label = NULL;
        q->op = Q_STORE;
        q->arg1.t = Q_VARIABLE;
        q->arg1.sym = varsym;
        q->arg2.t = Q_NONE;
        q->result.t = Q_SYMBOLIC;
        result_sym = new_symbol(env, true);
        q->result.register_priority = 0;
        q->result.sym = result_sym;
        quads->append_line(quads, q);
    } else if (expr->t == DIGITS_) {
        quadr_t *q = malloc(sizeof(quadr_t));
        q->t = QT_COPY;
        q->label = NULL;
        q->op = Q_STORE;
        q->arg1.t = Q_CONSTANT;
        q->arg1.constant = expr->digits;
        q->arg2.t = Q_NONE;
        q->result.t = Q_SYMBOLIC;
        q->result.register_priority = 0;
        result_sym = new_symbol(env, true);
        q->result.sym = result_sym;
        quads->append_line(quads, q);
    } else {
        quad_op_t quad_type = expr_to_quadop(expr->t);
        stack_item_t *op1 = stack->pop(stack);
        stack_item_t *op2 = stack->pop(stack);
        result_sym = new_symbol(env, true);
        if (op2->t != IDENT_ && op2->t != DIGITS_) {
            stack->push(stack, op2);
            resolve_destination(stack, backpatch, quads, env);
            quadr_t *q = generate_binary_expr_quad(
                quad_type,
                resolve_op(op1, quads, env),
                (backpatch->pop(backpatch))->ident,
                result_sym,
                env
            );
            quads->append_line(quads, q);
        } else {
            quadr_t *q = generate_binary_expr_quad(
                quad_type,
                resolve_op(op1, quads, env),
                resolve_op(op2, quads, env),
                result_sym,
                env
            );
            quads->append_line(quads, q);
        }
        stack_item_t *result = malloc(sizeof(stack_item_t));
        result->t = IDENT_;
        result->ident = result_sym;
        result->next = NULL;
        backpatch->push(backpatch, result);
    }

    return result_sym;
}


char* convert_expr_stack_to_quads(expr_stack_t *stack, quadblock_t *quads, env_t *env)
{
    expr_stack_t *backpatch = new_stack();
    return resolve_destination(stack, backpatch, quads, env);
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
    data->ir.t = ENV_SYMBOLIC;
    add_entry(env, sym, data);

    return sym;
}

quadr_t *generate_word_storage(char *name)
{
    quadr_t *storage = malloc(sizeof(quadr_t));
    storage->next = NULL;

    storage->t = QT_STORAGE;
    storage->op = Q_WORD;
    storage->label = name;
    storage->arg1.t = Q_NONE;
    storage->arg2.t = Q_NONE;
    storage->result.t = Q_NONE;

    return storage;
}

quadr_t *generate_binary_expr_quad(quad_op_t op, char *arg1, char *arg2, char *result, env_t *env)
{
    // Handle arithemetic operations.
    quadr_t *bin = malloc(sizeof(quadr_t));
    switch (op) {
        case Q_ADD:
        case Q_SUB:
            bin->t = QT_BINARY;
            bin->label = NULL;
            bin->op = op;
            bin->arg1.t = Q_SYMBOLIC;
            bin->arg1.sym = arg1;
            bin->arg1.register_priority = 0;
            bin->arg2.t = Q_SYMBOLIC;
            bin->arg2.sym = arg2;
            bin->arg2.register_priority = 0;
            bin->result.t = Q_SYMBOLIC;
            bin->result.sym = result;
            bin->result.register_priority = 0;
            return bin;
        case Q_MUL:
        case Q_DIV:
            bin->t = QT_BINARY;
            bin->label = NULL;
            bin->op = op;
            bin->arg1.t = Q_SYMBOLIC;
            bin->arg1.sym = arg1;
            bin->arg1.register_priority = 1;
            bin->arg2.t = Q_SYMBOLIC;
            bin->arg2.sym = arg2;
            bin->arg2.register_priority = 0;
            bin->result.t = Q_SYMBOLIC;
            bin->result.sym = result;
            bin->result.register_priority = 0;
            return bin;
        default:
            break;
    }

    // High-level overview of what's happening with logical
    // operations below.
    //
    //          CMP X, Y, Z
    //          COND_JMP s001
    //          MOV #0, Z
    //          BR s002
    // s001:    MOV #1, Z
    // s002:    NOP

    // Fall through to logical operations.
    switch (op) {
        case Q_GTE:
            op = Q_BGE;
            break;
        case Q_LTE:
            op = Q_BLE;
            break;
        case Q_EQ:
            op = Q_BEQ;
            break;
    }
    bin->t = QT_BINARY;
    bin->label = NULL;
    bin->op = Q_CMP;
    bin->arg1.t = Q_SYMBOLIC;
    bin->arg1.sym = arg1;
    bin->arg1.register_priority = 0;
    bin->arg2.t = Q_SYMBOLIC;
    bin->arg2.sym = arg2;
    bin->arg2.register_priority = 0;
    bin->result.t = Q_SYMBOLIC;
    bin->result.sym = result;
    bin->result.register_priority = 0;

    char *true_label = new_symbol(env, true);
    quadr_t *cond_jmp = malloc(sizeof(quadr_t));
    cond_jmp->t = QT_COND_JMP;
    cond_jmp->label = NULL;
    cond_jmp->op = op;
    cond_jmp->arg1.t = Q_LABEL;
    cond_jmp->arg1.sym = true_label;
    cond_jmp->arg2.t = Q_NONE;
    cond_jmp->result.t = Q_NONE;

    bin->next = cond_jmp;

    quadr_t *mov0 = malloc(sizeof(quadr_t));
    mov0->label = NULL;
    mov0->t = QT_COPY;
    mov0->op = Q_STORE;
    mov0->arg1.t = Q_CONSTANT;
    mov0->arg1.constant = 0;
    mov0->arg2.t = Q_NONE;
    mov0->result.t = Q_SYMBOLIC;
    mov0->result.sym = result;
    mov0->result.register_priority = 0;

    cond_jmp->next = mov0;

    char *false_label = new_symbol(env, true);
    quadr_t *jmp = malloc(sizeof(quadr_t));
    jmp->t = QT_UNCOND_JMP;
    jmp->op = Q_BR;
    jmp->label = NULL;
    jmp->arg1.t = Q_LABEL;
    jmp->arg1.sym = false_label;
    jmp->arg2.t = Q_NONE;
    jmp->result.t = Q_NONE;

    mov0->next = jmp;

    quadr_t *mov1 = malloc(sizeof(quadr_t));
    mov1->label = true_label;
    mov1->t = QT_COPY;
    mov1->op = Q_STORE;
    mov1->arg1.t = Q_CONSTANT;
    mov1->arg1.constant = 1;
    mov1->arg2.t = Q_NONE;
    mov1->result.t = Q_SYMBOLIC;
    mov1->result.sym = result;
    mov1->result.register_priority = 0;

    jmp->next = mov1;

    quadr_t *nop = malloc(sizeof(quadr_t));
    nop->label = false_label;
    nop->t = QT_NOP;
    nop->op = Q_NOP;
    nop->arg1.t = Q_NONE;
    nop->arg2.t = Q_NONE;
    nop->result.t = Q_NONE;

    mov1->next = nop;

    return bin;
}

quadr_t* generate_jump_target(char *label)
{
    quadr_t *jump_target = malloc(sizeof(quadr_t));
    jump_target->t = QT_NOP;
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
    cmp->t = QT_BINARY;
    cmp->label = NULL;
    cmp->op = Q_CMP;
    cmp->arg1.t = Q_CONSTANT;
    cmp->arg1.constant = 0;
    cmp->arg2.t = Q_SYMBOLIC;
    cmp->arg2.register_priority = 0;
    cmp->arg2.sym = sym;
    cmp->result.t = Q_SYMBOLIC;
    cmp->result.register_priority = 0;
    cmp->result.sym = new_symbol(env, true);
    return cmp;
}

quadr_t* generate_jmp(quad_type_t jmp_type, quad_op_t jmp_op, char *jump_label)
{
    quadr_t *branch = malloc(sizeof(quadr_t));
    branch->t = jmp_type;
    branch->label = NULL;
    branch->op = jmp_op;
    branch->arg1.t = Q_LABEL;
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
