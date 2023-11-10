#ifndef ENV_H
#define ENV_H

#define ENV_SIZE 100

#define UNINITIALIZED -1
#define SPILL -2

typedef enum env_data_type {
    ENV_FUNCTION,
    ENV_SYMBOLIC,
    ENV_VARIABLE
} env_data_type_t;

struct env_data;
typedef struct env_data env_data_t;
struct env_data {
    char *name;
    struct ir_ {
        env_data_type_t t;
        char *orig;
        char *sym;
        short reg;
    } ir;
    env_data_t *next;
};

typedef struct env {
    env_data_t *dict[ENV_SIZE];
} env_t;
#endif

unsigned long hash(char *s);

void init_env(env_t *env);

void add_entry(env_t *env, char *key, env_data_t *data);

env_data_t *lookup_entry(env_t *env, char *key);
