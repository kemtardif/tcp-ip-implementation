#ifndef _HASH_H_
#define _HASH_H_

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include "net.h"

#define SUBNET_CAPACITY 256
#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL
struct hash_entry
{
    char *key; //IP/MAC for Host+Router/Switch
    void *value; //MAC/Interface for Host+Router/Switch
};

struct hash_table
{
   struct hash_entry *entries;
   size_t capacity;
   size_t length;
};

struct hash_table *init_hash_table(size_t capacity);
void free_hash_table(struct hash_table *ht);
void *get_value(struct hash_table *ht, char *key);
char *set_value(struct hash_table *ht, char *key,void *value);
char *set_value_entry(struct hash_entry *entries, size_t capacity, char *key, void *value, size_t *length);
int expand_hash_table(struct hash_table *ht);
u_int64_t hash_key(char *key);


#endif