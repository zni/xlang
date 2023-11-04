#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

    data->next = NULL;

    // No collision, just insert the data.
    if (env->dict[loc] == NULL) {
        env->dict[loc] = data;
        return;
    }

    // Collision, check for next free in chain.
    env_data_t **chain = &(env->dict[loc]);
    while (*chain != NULL) {
        chain = &(*chain)->next;
    }
    *chain = data;
}

env_data_t* lookup_entry(env_t *env, char *key)
{
    unsigned int loc = hash(key) % ENV_SIZE;

    env_data_t *match = env->dict[loc];

    // Nothing inserted at location.
    if (match == NULL) {
        return NULL;
    }

    // Match with no entries in chain list.
    if (match->next == NULL) {
        return match;
    }

    // Check chain list for entry.
    do {
        if (strcmp(key, match->name) == 0)
            return match;

        match = match->next;
    } while (match != NULL);

    return NULL;
}