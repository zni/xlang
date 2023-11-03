#ifndef ENV_H
#define ENV_H

#define ENV_SIZE 100

struct env_data;
typedef struct env_data env_data_t;
struct env_data {
    char *name;
};

typedef struct env {
    env_data_t *dict[ENV_SIZE];
} env_t;
#endif

void init_env(env_t *env);

void add_entry(env_t *env, char *key, env_data_t *data);

env_data_t *lookup_entry(env_t *env, char *key);
