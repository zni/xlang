#ifndef IRGEN_H
#define IRGEN_H

#include "ast.h"
#include "env.h"

typedef enum quad_type {
    UNCOND_JMP,
    COND_JMP,
    COPY,
    BINARY,
    PROC_,
    NOP
} quad_type_t;

typedef enum quad_op {
    ADD__,
    SUB__,
    MUL__,
    DIV__,
    GTE__,
    LTE__,
    EQ__,
    CALL__,
    STORE__,
    GOTO__,
    CMP__,
    NOP__,
} quad_op_t;

typedef enum quad_result {
    CONSTANT,
    SYM,
    REG,
    NONE
} quad_result_t;


typedef struct quad_arg {
    quad_result_t t;
    union {
        char *sym;
        int constant;
        unsigned short reg;
    };
} quad_arg_t;

struct quad_;
typedef struct quad_ quadr_t;
struct quad_ {
    quad_type_t t;
    quad_op_t op;
    char *label;

    quad_arg_t arg1;
    quad_arg_t arg2;
    quad_arg_t result;

    quadr_t *next;
};

struct quad_program;
typedef struct quad_program quad_program_t;
struct quad_program {
    quadr_t *program;
    env_t *env;

    void (*append_quad)(quad_program_t*, quadr_t*);
};
 
struct stack_item;
typedef struct stack_item stack_item_t;
struct stack_item {
    exprval_t t;
    union {
        int digits;
        char *ident;
    };
    stack_item_t *next;
};

struct expr_stack;
typedef struct expr_stack expr_stack_t;
struct expr_stack {
    stack_item_t *next;

    void (*push)(expr_stack_t*, stack_item_t*);
    stack_item_t* (*pop)(expr_stack_t*);
};

void convert_to_quads(program_t*);
#endif