#ifndef IRGEN_H
#define IRGEN_H

#include "ast.h"
#include "env.h"

typedef enum quad_type {
    QT_UNCOND_JMP,
    QT_COND_JMP,
    QT_COPY,
    QT_BINARY,
    QT_PROCEDURE,
    QT_NOP
} quad_type_t;

typedef enum quad_op {
    Q_ADD,
    Q_SUB,
    Q_MUL,
    Q_DIV,
    Q_GTE,
    Q_BGE,
    Q_LTE,
    Q_BLE,
    Q_EQ,
    Q_BEQ,
    Q_BR,
    Q_CALL,
    Q_STORE,
    Q_GOTO,
    Q_CMP,
    Q_NOP,
    Q_RETURN
} quad_op_t;

typedef enum quad_result {
    Q_CONSTANT,
    Q_LABEL,
    Q_SYMBOLIC,
    Q_VARIABLE,
    Q_REGISTER,
    Q_NONE
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

struct quadblock;
typedef struct quadblock quadblock_t;
struct quadblock {
    quadr_t *lines;
    quadblock_t *next;

    void (*append_block)(quadblock_t*, quadblock_t*);
    void (*append_line)(quadblock_t*, quadr_t*);
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

quadblock_t* convert_to_quads(program_t*, env_t*);
#endif