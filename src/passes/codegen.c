#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "codegen.h"
#include "env.h"

void debug_lifetimes(interval_list_t*);

void print_assembly_blocks(assembly_block_t*);
void print_assembly(assembly_t*);
assembly_block_t* generate_assembly(quadblock_t*, env_t*);
assembly_t* generate_MOV(quadr_t*, env_t*);
assembly_t* generate_ADD(quadr_t*, env_t*);
assembly_t* generate_HALT();
void populate_operand(quad_arg_t*, asm_operand_t*, env_t*);

interval_list_t* calculate_liveness_intervals(quadblock_t*, env_t*);
void assign_registers(interval_list_t*, env_t*);
void expire_old_intervals(interval_t*, interval_list_t*, env_t*, register_pool_t*);
//void spill();

env_t* build_sym_env(env_t*);

interval_list_t* new_interval_list();
void add_interval(interval_list_t*, unsigned short, unsigned short, char*);
void remove_interval(interval_list_t*, char*);
interval_list_t* copy_interval_list(interval_list_t*);
void sort_by_lower(interval_list_t*);
void sort_by_upper(interval_list_t*);

set_t* new_set();
void add_item_to_set(set_t*, char*);
void remove_item_from_set(set_t*, char*);

void init_register_pool(register_pool_t*);
void add_register(register_pool_t*, short);
short remove_register(register_pool_t*);

assembly_block_t* new_assembly_block();
void append_assembly_block(assembly_block_t*, assembly_block_t*);
void append_assembly(assembly_block_t*, assembly_t*);
assembly_t* get_assembly(assembly_block_t*, unsigned int index);

void debug_lifetimes(interval_list_t *lifetimes)
{
    printf("LIFETIMES, n = %d\n", lifetimes->count);
    printf("------------------------\n");
    interval_t *lifetime = lifetimes->list;
    for (int i = 0; i < lifetimes->count; i++) {
        printf("[%d, %d] => %s\n", lifetime->lower, lifetime->upper, lifetime->sym);
        lifetime = lifetime->next;
    }

}

void print_assembly_blocks(assembly_block_t *block)
{
    assembly_t *code;
    assembly_block_t *b = block;
    while  (b != NULL) {
        for (int i = 0; i < b->instruction_count; i++) {
            code = get_assembly(b, i);
            print_assembly(code);
        }
        b = b->next;
    }
}

void print_assembly(assembly_t *code)
{
    if (code->label != NULL)
        printf("%s:\t", code->label);
    else
        printf("\t");

    switch (code->op) {
        case ASM_ADD:
            printf("ADD");
            break;
        case ASM_MOV:
            printf("MOV");
            break;
        default:
            printf("XXX");
    }

    switch (code->operand1.t) {
        case ASM_NONE:
            break;
        case ASM_CONSTANT:
            printf(" #%d", code->operand1.constant);
            break;
        case ASM_MEMORY:
            printf(" %s", code->operand1.memory);
            break;
        case ASM_REGISTER:
            printf(" R%d", code->operand1.reg);
            break;
    }

    switch (code->operand2.t) {
        case ASM_NONE:
            break;
        case ASM_CONSTANT:
            printf(", #%d", code->operand2.constant);
            break;
        case ASM_MEMORY:
            printf(", %s", code->operand2.memory);
            break;
        case ASM_REGISTER:
            printf(", R%d", code->operand2.reg);
            break;
    }
    putchar('\n');
}

assembly_block_t *generate_assembly(quadblock_t *block, env_t *env)
{
    assembly_block_t *codeblock = new_assembly_block();
    assembly_t *code;
    quadr_t *line = block->lines;
    while (line != NULL) {
        switch (line->op) {
            case Q_STORE:
                code = generate_MOV(line, env);
                break;
            case Q_ADD:
                code = generate_ADD(line, env);
                break;
            default:
                break;
        }
        line = line->next;
        append_assembly(codeblock, code);
    }

    return codeblock;
}

assembly_t* generate_MOV(quadr_t *line, env_t *env)
{
    assembly_t *mov = malloc(sizeof(assembly_t));
    mov->next = NULL;

    mov->op = ASM_MOV;
    mov->label = line->label;
    populate_operand(&line->arg1, &mov->operand1, env);
    populate_operand(&line->result, &mov->operand2, env);

    return mov;
}

assembly_t* generate_ADD(quadr_t *line, env_t *env)
{
    assembly_t *add = malloc(sizeof(assembly_t));
    add->next = NULL;

    add->op = ASM_ADD;
    add->label = line->label;
    populate_operand(&line->arg1, &add->operand1, env);
    populate_operand(&line->arg2, &add->operand2, env);

    assembly_t *mov = malloc(sizeof(assembly_t));
    mov->next = NULL;

    mov->op = ASM_MOV;
    populate_operand(&line->arg2, &mov->operand1, env);
    populate_operand(&line->result, &mov->operand2, env);

    add->next = mov;
    return add;
}

assembly_t* generate_HALT()
{
    assembly_t *halt = malloc(sizeof(assembly_t));
    halt->next = NULL;

    halt->op = ASM_HALT;
    halt->operand1.t = ASM_NONE;
    halt->operand2.t = ASM_NONE;
    return halt;
}

void populate_operand(quad_arg_t *qa, asm_operand_t *op, env_t *env)
{
    env_data_t *lookup;
    switch (qa->t) {
        case CONSTANT:
            op->t = ASM_CONSTANT;
            op->constant = qa->constant;
            break;
        case SYM:
            lookup = lookup_entry(env, qa->sym);
            if (lookup != NULL && lookup->ir.reg >= 0) {
                op->t = ASM_REGISTER;
                op->reg = lookup->ir.reg;
            } else {
                op->t = ASM_MEMORY;
                op->memory = qa->sym;
            }
            break;
        case VARIABLE:
            op->t = ASM_MEMORY;
            op->memory = qa->sym;
            break;
        default:
            break;
    }
}

void generate_code(quadblock_t *blocks, env_t *env)
{
    //env_t *sym_env = build_sym_env(env);
    quadblock_t *b = blocks->next;
    while (b != NULL) {
        interval_list_t *lifetimes = calculate_liveness_intervals(b, env);
        sort_by_lower(lifetimes);
        debug_lifetimes(lifetimes);
        assign_registers(lifetimes, env);
        b = b->next;
    }

    assembly_block_t *main_block = NULL;
    b = blocks->next;
    while (b != NULL) {
        if (main_block == NULL)
            main_block = generate_assembly(b, env);
        else
            append_assembly_block(main_block, generate_assembly(b, env));

        b = b->next;
    }

    print_assembly_blocks(main_block);
}

interval_list_t* calculate_liveness_intervals(quadblock_t *block, env_t *env)
{
    interval_list_t *lifetimes = new_interval_list();
    set_t *syms = new_set();

    quadr_t *line = block->lines;
    while (line != NULL) {
        if (line->arg1.t == SYM) {
            add_item_to_set(syms, line->arg1.sym);
        }

        if (line->arg2.t == SYM) {
            add_item_to_set(syms, line->arg2.sym);
        }

        if (line->result.t == SYM) {
            add_item_to_set(syms, line->result.sym);
        }

        line = line->next;
    }

    unsigned short begin = 0;
    set_element_t *element;
    for (int i = 0; i < SET_SIZE; i++) {
        element = syms->s[i];
        if (element == NULL) {
            continue;
        }
        while (element != NULL) {
            begin = 0;
            line = block->lines;
            while (line != NULL) {
                if (((line->arg1.t == SYM) && (strcmp(line->arg1.sym, element->sym) == 0)) ||
                    ((line->arg2.t == SYM) && (strcmp(line->arg2.sym, element->sym) == 0))  ||
                    ((line->result.t == SYM) && (strcmp(line->result.sym, element->sym) == 0))) {
                    break;
                } else {
                    begin++;
                }

                line = line->next;
            }

            unsigned short end = begin;
            unsigned short last_end_index = end;
            while (line != NULL) {
                if (((line->arg1.t == SYM) && (strcmp(line->arg1.sym, element->sym) == 0)) ||
                    ((line->arg2.t == SYM) && (strcmp(line->arg2.sym, element->sym) == 0))  ||
                    ((line->result.t == SYM) && (strcmp(line->result.sym, element->sym) == 0))) {
                    last_end_index = end;
                    end++;
                } else {
                    end++;
                }

                line = line->next;
            }

            add_interval(lifetimes, begin, last_end_index, element->sym);

            element = element->next;
        }
    }

    return lifetimes;
}

void assign_registers(interval_list_t *lifetimes, env_t *env)
{
    register_pool_t pool;
    init_register_pool(&pool);

    interval_list_t *active = new_interval_list();
    interval_t *current = lifetimes->list;
    char *lifetime_sym = current->sym;
    for (int i = 0; i < lifetimes->count; i++) {
        expire_old_intervals(current, active, env, &pool);
        if (active->count == 6) {
            //spill();
            printf("spilling => %s\n", lifetime_sym);
        } else {
            env_data_t *sym = lookup_entry(env, lifetime_sym);
            if (sym != NULL) {
                short reg = remove_register(&pool);
                sym->ir.reg = reg;
                add_interval(active, current->lower, current->upper, lifetime_sym);
            }
        }
        current = current->next;
        if (current == NULL)
            break;
        lifetime_sym = current->sym;
    }
}

void expire_old_intervals(interval_t *interval, interval_list_t *active, env_t *env, register_pool_t *pool)
{
    if (active->count == 0) {
        return;
    }

    interval_list_t *endtimes = active;
    sort_by_upper(endtimes);
    interval_t *end = endtimes->list;
    for (int i = 0; i < endtimes->count; i++) {
        if (end->upper >= interval->lower) {
            return;
        }
        break;
    }

    sort_by_lower(active);
    char *sym = end->sym;
    remove_interval(active, end->sym);
    env_data_t *d = lookup_entry(env, sym);
    add_register(pool, d->ir.reg);
}

env_t* build_sym_env(env_t *env)
{
    env_t *sym_env = malloc(sizeof(env_t));

    env_data_t *entry, *chain;
    for (int i = 0; i < ENV_SIZE; i++) {
        if (env->dict[i] == NULL)
            continue;

        entry = env->dict[i];
        if (entry->ir.t == SYMBOLIC) {
            entry->ir.reg = UNINITIALIZED;
            add_entry(sym_env, entry->ir.sym, entry);
        }

        chain = env->dict[i]->next;
        while (chain != NULL) {
            if (chain->ir.t == SYMBOLIC) {
                chain->ir.reg = UNINITIALIZED;
                add_entry(sym_env, chain->ir.sym, chain);
            }

            chain = chain->next;
        }
    }



    return sym_env;
}

interval_list_t* new_interval_list()
{
    interval_list_t *list = malloc(sizeof(interval_list_t));
    list->list = NULL;
    list->count = 0;

    return list;
}

void add_interval(interval_list_t *list, unsigned short lower, unsigned short upper, char *sym)
{
    interval_t *interval = malloc(sizeof(interval_t));
    interval->lower = lower;
    interval->upper = upper;
    interval->sym = sym;

    if (list->list == NULL) {
        list->list = interval;
        interval->next = NULL;
        list->count += 1;
    } else {
        interval_t **next = &(list->list);
        while (*next != NULL) {
            next = &(*next)->next;
        }

        *next = interval;
        (*next)->next = NULL;
        list->count += 1;
    }
}

void remove_interval(interval_list_t *list, char *sym)
{
    if (list->count == 0) {
        return;
    }

    interval_t *tmp;
    interval_t **c = &(list->list);
    if (strcmp(sym, (*c)->sym) == 0) {
        tmp = *c;
        if (list->count > 1) {
            list->list = list->list->next;
        } else {
            list->list = NULL;
        }
        free(tmp);
        tmp = NULL;
        list->count--;
        return;
    }

    interval_t **p;
    while (*c != NULL) {
        p = &(*c);
        c = &(*c)->next;
        if (strcmp(sym, (*c)->sym) == 0) {
            (*p)->next = (*c)->next;
            tmp = *c;
            free(tmp);
            tmp = NULL;
            list->count--;
            return;
        }
    }
}

void sort_by_lower(interval_list_t *list)
{
    if (list->count == 0 || list->count == 1) {
        return;
    }

    interval_t **node;
    interval_t *n1, *n2, *tmp;
    for (int round = 0; round < list->count; round++) {
        node = &(list->list);
        for (int n = 0; n < list->count; n++) {
            if ((*node)->lower > (*node)->next->lower) {
                n1 = *node;
                n2 = (*node)->next;
                tmp = n2->next;
                n2->next = n1;
                n1->next = tmp;
                *node = n2;
            }
            node = &(*node)->next;
            if (*node == NULL || (*node)->next == NULL) {
                break;
            }
        }
    }
}

void sort_by_upper(interval_list_t *list)
{
    if (list->count == 0 || list->count == 1) {
        return;
    }

    interval_t **node;
    interval_t *n1, *n2, *tmp;
    for (int round = 0; round < list->count; round++) {
        node = &(list->list);
        for (int n = 0; n < list->count; n++) {
            if ((*node)->upper > (*node)->next->upper) {
                n1 = *node;
                n2 = (*node)->next;
                tmp = n2->next;
                n2->next = n1;
                n1->next = tmp;
                *node = n2;
            }
            node = &(*node)->next;
            if (*node == NULL || (*node)->next == NULL) {
                break;
            }
        }
    }
}

set_t* new_set()
{
    set_t *set = malloc(sizeof(set_t));
    for (int i = 0; i < SET_SIZE; i++) {
        set->s[i] = NULL;
    }

    return set;
}

void add_item_to_set(set_t *set, char *sym)
{
    unsigned int loc = hash(sym) % SET_SIZE;

    set_element_t *element = malloc(sizeof(set_element_t));
    element->next = NULL;
    element->sym = sym;

    // No collision, just insert the data.
    if (set->s[loc] == NULL) {
        set->s[loc] = element;
        return;
    }

    // Element is already in the set.
    if (strcmp(set->s[loc]->sym, element->sym) == 0) {
        return;
    }

    // Collision, check for next free in chain.
    set_element_t **chain = &(set->s[loc]);
    while (*chain != NULL) {
        chain = &(*chain)->next;
    }
    *chain = element;
}

void remove_item_from_set(set_t *set, char *sym)
{
    unsigned int loc = hash(sym) % SET_SIZE;

    if (set->s[loc] == NULL) {
        return;
    } else {
        set_element_t *element = set->s[loc];
        if (strcmp(sym, element->sym) == 0) {
            set->s[loc] = element->next;
            free(element);
            element = NULL;
        } else {
            set_element_t *tmp;
            while (element != NULL) {
                tmp = element;
                element = element->next;
                if (strcmp(sym, element->sym) == 0) {
                    tmp->next = element->next;
                    free(element);
                    element = NULL;
                    break;
                }
            }
        }
    }
}

void init_register_pool(register_pool_t *pool)
{
    for (int r = 0; r < 6; r++) {
        pool->pool[r] = r;
    }
    pool->index = 0;
}

void add_register(register_pool_t *pool, short reg)
{
    pool->pool[reg] = reg;
}

short remove_register(register_pool_t *pool)
{
    short reg;
    for (int i = 0; i < 6; i++) {
        if (pool->pool[i] != SPILL) {
            reg = pool->pool[i];
            pool->pool[i] = SPILL;
            return reg;
        }
    }

    return SPILL;
}

assembly_block_t* new_assembly_block()
{
    assembly_block_t *block = malloc(sizeof(assembly_block_t));
    block->next = NULL;
    block->instruction_count = 0;
    block->code = NULL;

    return block;
}

void append_assembly_block(assembly_block_t *block_head, assembly_block_t *b)
{
    assembly_block_t **next = &(b->next);
    while (*next != NULL)
        next = &(*next)->next;
    *next = b;
}

void append_assembly(assembly_block_t *block, assembly_t *code)
{
    unsigned int instructions = 0;
    assembly_t *c = code;
    while (c != NULL) {
        instructions++;
        c = c->next;
    }

    assembly_t **next = &(block->code);
    while (*next != NULL)
        next = &(*next)->next;
    *next = code;

    block->instruction_count += instructions;
}

assembly_t* get_assembly(assembly_block_t *block, unsigned int index)
{
    assembly_t *b = block->code;
    for (int i = 0; i < block->instruction_count; i++) {
        if (i == index)
            break;

        b = b->next;
    }

    return b;
}
