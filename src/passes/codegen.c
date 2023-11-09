#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "codegen.h"
#include "env.h"

void debug_lifetimes(interval_list_t*);

void generate_assembly(quadblock_t*, env_t*);
void generate_MOV(quadr_t*, env_t*);
void generate_ADD(quadr_t*, env_t*);

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

void generate_assembly(quadblock_t *block, env_t *env)
{
    quadr_t *line = block->lines;
    while (line != NULL) {
        switch (line->op) {
            case STORE__:
                generate_MOV(line, env);
                break;
            case ADD__:
                generate_ADD(line, env);
                break;
            default:
                break;
        }
        line = line->next;
    }
}

void generate_MOV(quadr_t *line, env_t *env)
{
    env_data_t *lookup;
    printf("\tMOV ");
    switch (line->arg1.t) {
        case CONSTANT:
            printf("#%d, ", line->arg1.constant);
            break;
        case SYM:
            lookup = lookup_entry(env, line->arg1.sym);
            if (lookup->ir.reg >= 0) {
                printf("R%d, ", lookup->ir.reg);
            } else {
                printf("%s, ", lookup->ir.sym);
            }
            break;
        case VARIABLE:
            printf("%s, ", line->arg1.sym);
            break;
        default:
            break;
    }
    switch (line->result.t) {
        case CONSTANT:
            printf("#%d\n", line->result.constant);
            break;
        case SYM:
            lookup = lookup_entry(env, line->result.sym);

            if (lookup != NULL && lookup->ir.reg >= 0) {
                printf("R%d\n", lookup->ir.reg);
            } else {
                printf("%s\n", line->result.sym);
            }
            break;
        case VARIABLE:
            printf("%s\n", line->result.sym);
            break;
        default:
            break;
    }
}

void generate_ADD(quadr_t *line, env_t *env)
{
    env_data_t *lookup;
    printf("\tADD ");
    switch (line->arg1.t) {
        case CONSTANT:
            printf("#%d, ", line->arg1.constant);
            break;
        case SYM:
            lookup = lookup_entry(env, line->arg1.sym);
            if (lookup->ir.reg >= 0) {
                printf("R%d, ", lookup->ir.reg);
            } else {
                printf("%s, ", lookup->ir.sym);
            }
            break;
        case VARIABLE:
            printf("%s, ", line->arg1.sym);
            break;
        default:
            break;
    }
    switch (line->arg2.t) {
        case SYM:
            lookup = lookup_entry(env, line->arg2.sym);
            if (lookup != NULL && lookup->ir.reg >= 0) {
                printf("R%d, ", lookup->ir.reg);
            } else {
                printf("%s, ", line->arg2.sym);
            }
            break;
        case VARIABLE:
            printf("%s, ", line->arg2.sym);
            break;
        default:
            break;
    }
    switch (line->result.t) {
        case CONSTANT:
            printf("#%d\n", line->result.constant);
            break;
        case SYM:
            lookup = lookup_entry(env, line->result.sym);

            if (lookup != NULL && lookup->ir.reg >= 0) {
                printf("R%d\n", lookup->ir.reg);
            } else {
                printf("%s\n", line->result.sym);
            }
            break;
        case VARIABLE:
            printf("%s\n", line->result.sym);
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

    b = blocks->next;
    while (b != NULL) {
        generate_assembly(b, env);
        b = b->next;
    }
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
        list->count += 1;
    }
}

void remove_interval(interval_list_t *list, char *sym)
{
    if (list->count == 0) {
        return;
    }

    interval_t **c = &(list->list);
    if (strcmp(sym, (*c)->sym) == 0) {
        interval_t *tmp = *c;
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
            p = &(*c)->next;
            free(*c);
            *c = NULL;
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
