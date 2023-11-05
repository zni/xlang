#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "irgen.h"

unsigned int SYMBOL_INDEX = 0;

void convert_blocks_to_quads(block_t *block, quad_program_t *quads);
void convert_decs_to_quads(var_dec_t *dec, quad_program_t *quads);
void convert_stmts_to_quads(statement_t *stmt, quad_program_t *quads);
void convert_expr_to_stack(expression_t *node, expr_stack_t *stack);
char* convert_expr_stack_to_quads(expr_stack_t*, quad_program_t*);
char* new_symbol();
quadr_t* generate_binary_expr_quad(quad_op_t t, char *arg1, char *arg2, char *result);
char* lookup_sym(quad_program_t*, char*);
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

void append_quad(quad_program_t *q, quadr_t *quad)
{
    if (q->program == NULL) {
        q->program = quad;
        quad->next = NULL;
    } else {
        quadr_t **tmp = &(q->program);
        while (*tmp != NULL)
            tmp = &(*tmp)->next;

        *tmp = quad;
    }
}

quad_program_t* new_quad_program()
{
    quad_program_t *quads = malloc(sizeof(quad_program_t));
    quads->env = malloc(sizeof(env_t));
    quads->program = NULL;
    quads->append_quad = &append_quad;
    return quads;
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

void debug_quads(quad_program_t *q)
{
    quadr_t *tmp = q->program;
    while (tmp != NULL) {
        switch (tmp->op) {
        case ADD__:
            printf("ADD\t%s,\t%s,\t%s\n", tmp->arg1_s, tmp->arg2, tmp->result);
            break;
        case SUB__:
            printf("SUB\t%s, %s, %s\n", tmp->arg1_s, tmp->arg2, tmp->result);
            break;
        case MUL__:
            printf("MUL\t%s, %s, %s\n", tmp->arg1_s, tmp->arg2, tmp->result);
            break;
        case DIV__:
            printf("DIV\t%s, %s, %s\n", tmp->arg1_s, tmp->arg2, tmp->result);
            break;
        case GTE__:
            printf("GTE\t%s,\t%s,\t%s\n", tmp->arg1_s, tmp->arg2, tmp->result);
            break;
        case LTE__:
            printf("LTE\t%s, %s, %s\n", tmp->arg1_s, tmp->arg2, tmp->result);
            break;
        case EQ__:
            printf("EQ\t%s, %s, %s\n", tmp->arg1_s, tmp->arg2, tmp->result);
            break;
        case CMP__:
            printf("CMP\t");
            if (tmp->arg1_t == CONSTANT) {
                printf("%d,\t", tmp->arg1_d);
            } else {
                printf("%s,\t", tmp->arg1_s);
            }
            printf("%s,\t", tmp->arg2);
            if (tmp->res_t == CONSTANT) {
                printf("%d\n", tmp->value);
            } else {
                printf("%s\n", tmp->result);
            }
            break;
        case NOP__:
            printf("%s:\tNOP\n", tmp->label);
            break;
        case GOTO__:
            printf("\tGOTO\t%s\n", tmp->arg1_s);
            break;
        case CALL__:
            printf("CALL "); // TODO
            break;
        case STORE__:
            if (tmp->arg1_t == CONSTANT) {
                printf("STORE\t%d,\t\t%s\n", tmp->arg1_d, tmp->result);
            } else if (tmp->arg1_t == SYM) {
                printf("STORE\t%s,\t\t%s\n", tmp->arg1_s, tmp->result);
            }
            break;
        }
        tmp = tmp->next;
    }
}

void convert_to_quads(program_t *program)
{
    quad_program_t *quads = new_quad_program();
    convert_blocks_to_quads(program->blocks, quads);

    debug_quads(quads);
}

void convert_blocks_to_quads(block_t *block, quad_program_t *quads)
{
    while (block != NULL) {
        if (block->t == PROCEDURE) {
            env_data_t *data = malloc(sizeof(env_data_t));
            data->name = block->procedure.name;
            data->ir.orig = block->procedure.name;
            data->ir.sym = new_symbol();
            add_entry(quads->env, block->procedure.name, data);

            convert_blocks_to_quads(block->procedure.context, quads);
        } else if (block->t == DECLARATION) {
            convert_decs_to_quads(block->decs, quads);
        } else if (block->t == STATEMENT) {
            convert_stmts_to_quads(block->stmts, quads);
        }
        block = block->next;
    }
}

void convert_decs_to_quads(var_dec_t *dec, quad_program_t *quads)
{
    while (dec != NULL) {
        env_data_t *data = malloc(sizeof(env_data_t));
        data->name = dec->var;
        data->ir.orig = dec->var;
        data->ir.sym = new_symbol();
        add_entry(quads->env, dec->var, data);

        dec = dec->next;
    }
}

void convert_stmts_to_quads(statement_t *stmt, quad_program_t *quads)
{
    if (stmt == NULL) {
        return;
    } else if (stmt->t == ASSIGN_) {
        char *sym = lookup_sym(quads, stmt->assign.var);
        expr_stack_t *stack = new_stack();
        convert_expr_to_stack(stmt->assign.value, stack);
        debug_stack(stack);
        char *result_sym = convert_expr_stack_to_quads(stack, quads);
        quadr_t *assign = malloc(sizeof(quadr_t));
        assign->t = COPY;
        assign->op = STORE__;
        assign->arg1_t = SYM;
        assign->arg1_s = result_sym;
        assign->res_t = SYM;
        assign->result = sym;
        quads->append_quad(quads, assign);
    } else if (stmt->t == IF_) {
        expr_stack_t *stack = new_stack();
        convert_expr_to_stack(stmt->if_.cond, stack);
        char *result_sym = convert_expr_stack_to_quads(stack, quads);
        quadr_t *cmp = malloc(sizeof(quadr_t));
        cmp->t = BINARY;
        cmp->op = CMP__;
        cmp->arg1_t = CONSTANT;
        cmp->arg1_d = 0;
        cmp->arg2 = result_sym;
        cmp->res_t = SYM;
        cmp->result = new_symbol();
        quads->append_quad(quads, cmp);
        
        char *jump_label = new_symbol();
        quadr_t *branch = malloc(sizeof(quadr_t));
        branch->t = COND_JMP;
        branch->op = GOTO__;
        branch->arg1_t = SYM;
        branch->arg1_s = jump_label;
        branch->arg2 = NULL;
        branch->res_t = SYM;
        branch->result = NULL;
        quads->append_quad(quads, branch);

        convert_stmts_to_quads(stmt->if_.body, quads);

        quadr_t *jump_target = malloc(sizeof(quadr_t));
        jump_target->t = NOP;
        jump_target->label = jump_label;
        jump_target->op = NOP__;
        jump_target->arg1_t = SYM;
        jump_target->arg1_s = NULL;
        jump_target->arg2 = NULL;
        jump_target->res_t = SYM;
        jump_target->result = NULL;
        quads->append_quad(quads, jump_target);

    } else if (stmt->t == WHILE_) {
        expr_stack_t *stack = new_stack();
        convert_expr_to_stack(stmt->while_.cond, stack);
        convert_stmts_to_quads(stmt->while_.body, quads);
    } else if (stmt->t == BEGIN_) {
        convert_stmts_to_quads(stmt->begin_.body, quads);
    } else if (stmt->t == CALL_) {
        /* TODO */
    }
    convert_stmts_to_quads(stmt->next, quads);
}

char* resolve_op(stack_item_t *op, quad_program_t *quads)
{
    char *sym = new_symbol();
    if (op->t == DIGITS_) {
        quadr_t *q = malloc(sizeof(quadr_t));
        q->t = COPY;
        q->op = STORE__;
        q->arg1_t = CONSTANT;
        q->arg1_d = op->digits;
        q->arg2 = NULL;
        q->res_t = SYM;
        q->result = sym;
        quads->append_quad(quads, q);
    } else if (op->t == IDENT_) {
        char *varsym = lookup_sym(quads, op->ident);
        quadr_t *q = malloc(sizeof(quadr_t));
        q->t = COPY;
        q->op = STORE__;
        q->arg1_t = SYM;
        q->arg1_s = varsym;
        q->arg2 = NULL;
        q->res_t = SYM;
        q->result = sym;
        quads->append_quad(quads, q);
    }

    return sym;
}

void resolve_destination(stack_item_t *expr, expr_stack_t *stack, expr_stack_t *backpatch, quad_program_t *quads)
{
    if (expr->t == IDENT_) {
        char *varsym = lookup_sym(quads, expr->ident);
        quadr_t *q = malloc(sizeof(quadr_t));
        q->t = COPY;
        q->op = STORE__;
        q->arg1_t = SYM;
        q->arg1_s = varsym;
        q->res_t = SYM;
        q->result = new_symbol();
        quads->append_quad(quads, q);
    } else if (expr->t == DIGITS_) {
        quadr_t *q = malloc(sizeof(quadr_t));
        q->t = COPY;
        q->op = STORE__;
        q->arg1_t = CONSTANT;
        q->arg1_d = expr->digits;
        q->res_t = SYM;
        q->result = new_symbol();
        quads->append_quad(quads, q);
    } else {
        quad_op_t quad_type = expr_to_quadop(expr->t);
        stack_item_t *op1 = stack->pop(stack);
        stack_item_t *op2 = stack->pop(stack);
        char *result_sym = new_symbol();
        if (op2->t != IDENT_ && op2->t != DIGITS_) {
            resolve_destination(op2, stack, backpatch, quads);
            quadr_t *q = malloc(sizeof(quadr_t));
            q->t = BINARY;
            q->op = quad_type;
            q->arg1_t = SYM;
            q->arg1_s = resolve_op(op1, quads);
            q->arg2 = (backpatch->pop(backpatch))->ident;
            q->res_t = SYM;
            q->result = result_sym;
            quads->append_quad(quads, q);
        } else {
            quadr_t *q = malloc(sizeof(quadr_t));
            q->t = BINARY;
            q->op = quad_type;
            q->arg1_t = SYM;
            q->arg1_s = resolve_op(op1, quads);
            q->arg2 = resolve_op(op2, quads);
            q->res_t = SYM;
            q->result = result_sym;
            quads->append_quad(quads, q);
        }
        stack_item_t *result = malloc(sizeof(stack_item_t));
        result->t = IDENT_;
        result->ident = result_sym;
        result->next = NULL;
        backpatch->push(backpatch, result);
    }
}


char* convert_expr_stack_to_quads(expr_stack_t *stack, quad_program_t *quads)
{
    char *result_sym;
    while (stack->next != NULL) {
        stack_item_t *top = stack->pop(stack);
        if (top->t == IDENT_) {
            char *varsym = lookup_sym(quads, top->ident);
            result_sym = new_symbol();
            quadr_t *q = malloc(sizeof(quadr_t));
            q->t = COPY;
            q->op = STORE__;
            q->arg1_t = SYM;
            q->arg1_s = varsym;
            q->res_t = SYM;
            q->result = result_sym;
            quads->append_quad(quads, q);
        } else if (top->t == DIGITS_) {
            result_sym = new_symbol();
            quadr_t *q = malloc(sizeof(quadr_t));
            q->t = COPY;
            q->op = STORE__;
            q->arg1_t = CONSTANT;
            q->arg1_d = top->digits;
            q->res_t = SYM;
            q->result = result_sym;
            quads->append_quad(quads, q);
        } else {
            quad_op_t quad_type = expr_to_quadop(top->t);
            stack_item_t *op1 = stack->pop(stack);
            stack_item_t *op2 = stack->pop(stack);
            if (op2->t != IDENT_ && op2->t != DIGITS_) {
                expr_stack_t *backpatch = new_stack();
                resolve_destination(op2, stack, backpatch, quads);
                result_sym = new_symbol();
                quadr_t *q = malloc(sizeof(quadr_t));
                q->t = BINARY;
                q->op = quad_type;
                q->arg1_t = SYM;
                q->arg1_s = resolve_op(op1, quads);
                q->arg2 = (backpatch->pop(backpatch))->ident;
                q->res_t = SYM;
                q->result = result_sym;
                quads->append_quad(quads, q);
            } else {
                result_sym = new_symbol();
                quadr_t *q = malloc(sizeof(quadr_t));
                q->t = BINARY;
                q->op = quad_type;
                q->arg1_t = SYM;
                q->arg1_s = resolve_op(op1, quads);
                q->arg2 = resolve_op(op2, quads);
                q->res_t = SYM;
                q->result = result_sym;
                quads->append_quad(quads, q);
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

char* new_symbol()
{
    char *sym = malloc(sizeof(char) * 8);
    sprintf(sym, "s%03u", SYMBOL_INDEX);
    SYMBOL_INDEX++;

    return sym;
}

quadr_t *generate_binary_expr_quad(quad_op_t op, char *arg1, char *arg2, char *result)
{
    quadr_t *bin = malloc(sizeof(quadr_t));
    bin->t = BINARY;
    bin->op = op;
    bin->arg1_t = SYM;
    bin->arg1_s = arg1;
    bin->arg2 = arg2;
    bin->res_t = SYM;
    bin->result = result;

    return bin;
}

char* lookup_sym(quad_program_t *quads, char *key)
{
    env_data_t *var = lookup_entry(quads->env, key);
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
        return ADD__;
    case SUB:
        return SUB__;
    case MUL:
        return MUL__;
    case DIV:
        return DIV__;
    case LTE_:
        return LTE__;
    case GTE_:
        return GTE__;
    case EQ_:
        return EQ__;

    case IDENT_:
    case DIGITS_:
    default:
        return -1;
    }
}
