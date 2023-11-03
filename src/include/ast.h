#ifndef AST_H
#define AST_H
typedef enum type {
    VAR_DEC,
    IF_,
    ASSIGN_,
    WHILE_,
    BEGIN_,
    CALL_
} type_t;

typedef enum exprval {
    IDENT_,
    DIGITS_,
    ADD,
    SUB,
    MUL,
    DIV,
    LTE_,
    GTE_,
    EQ_
} exprval_t;

typedef enum block_type {
    PROCEDURE,
    DECLARATION,
    STATEMENT
} block_type_t;

struct expression;
typedef struct expression expression_t;
struct expression {
    exprval_t t;
    union {
        int digits;
        char *ident;
    };
    expression_t *left;
    expression_t *right;
};

struct statement;
typedef struct statement statement_t;
struct statement {
    type_t t;
    union {
        struct if_stmt {
            expression_t *cond;
            statement_t *body;
        } if_;
        struct while_stmt {
            expression_t *cond;
            statement_t *body;
        } while_;
        struct assign_stmt {
            char *var;
            expression_t *value;
        } assign;
        struct begin_stmt {
            statement_t *body;
        } begin_;
        struct call_stmt {
            char *var;
        } call_;
    };
    statement_t *next;
};

struct var_dec;
typedef struct var_dec var_dec_t;
struct var_dec {
    char *var;
    var_dec_t *next;
};

struct block;
typedef struct block block_t;
struct block {
    block_type_t t;
    union {
        struct p {
            char *name;
            block_t *context;
        } procedure;
        var_dec_t *decs;
        statement_t *stmts;
    };
    block_t *next;
};

struct program;
typedef struct program program_t;
struct program {
    block_t *blocks;
};
#endif