#ifndef CODEGEN_H
#define CODEGEN_H

#include "irgen.h"

#define SET_SIZE 100

/*
 * Register Allocation Structures
 */

struct interval;
typedef struct interval interval_t;
struct interval {
    unsigned short upper;
    unsigned short lower;
    char *sym;

    interval_t *next;
};

typedef struct interval_list {
    unsigned short count;
    interval_t *list;
} interval_list_t;

struct set_element;
typedef struct set_element set_element_t;
struct set_element {
    char *sym;
    set_element_t *next;
};

typedef struct set {
    set_element_t *s[SET_SIZE];
} set_t;

typedef struct register_pool {
    short pool[6];
    short index;
} register_pool_t;

/*
 * Assembly Structures
 */

typedef enum asm_op {
    ASM_MOV,
    ASM_ADD,
    ASM_SUB,
    ASM_DIV,
    ASM_MUL,
    ASM_CMP,
    ASM_JMP,
    ASM_BEQ,
    ASM_BGE,
    ASM_BLE,
    ASM_BNE,
    ASM_BR,
    ASM_JSR,
    ASM_RTS,
    ASM_HALT,
    ASM_NOP
} asm_op_t;

typedef enum asm_arg {
    ASM_LABEL,
    ASM_MEMORY,
    ASM_REGISTER,
    ASM_CONSTANT,
    ASM_NONE
} asm_arg_t;

typedef struct asm_operand {
    asm_arg_t t;
    union {
        char *memory;
        short reg;
        unsigned short constant;
    };
} asm_operand_t;

typedef char* asm_label_t;

struct assembly;
typedef struct assembly assembly_t;
struct assembly {
    asm_label_t label;
    asm_op_t op;
    asm_operand_t operand1;
    asm_operand_t operand2;

    assembly_t *next;
};

struct assembly_block;
typedef struct assembly_block assembly_block_t;
struct assembly_block {
    assembly_t *code;
    unsigned int instruction_count;

    assembly_block_t *next;
};

void generate_code(quadblock_t*, env_t*);

#endif