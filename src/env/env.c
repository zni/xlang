#include <stdio.h>
#include <stdlib.h>
#include "env.h"

// From http://www.cse.yorku.ca/%7Eoz/hash.html .
unsigned long hash(char *s)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *s++)) {
        hash = (hash << 5) + hash + c;
    }

    return hash;
}

void init_env(env_t *env)
{
    for (int i = 0; i < ENV_SIZE; i++) {
        env->dict[i] = NULL;
    }
}

void add_entry(env_t *env, char *key, env_data_t *data)
{
    unsigned int loc = hash(key) % ENV_SIZE;

    if (env->dict[loc] != NULL) {
        printf("collision\n");
    } else {
        env->dict[loc] = data;
    }
}

env_data_t* lookup_entry(env_t *env, char *key)
{
    unsigned int loc = hash(key) % ENV_SIZE;

    return env->dict[loc];
}