#include "hash.h"

struct hash_table *init_hash_table(size_t capacity)
{
    struct hash_table *ht;
    int i;

    if((ht = malloc(sizeof(struct hash_table))) == NULL)
        return NULL;
    if((ht->entries = malloc(capacity * sizeof(struct hash_entry))) == NULL)
    {
        free(ht);
        return NULL;
    }
    for(i = 0; i < capacity; i++)
    {
         ht->entries[i].key = NULL;
         ht->entries[i].value = NULL;
    }
    ht->capacity = capacity;
    ht->length = 0;
    return ht;
}

void free_hash_table(struct hash_table *ht)
{   int i;
    if(!ht)
        return;
    for(i = 0; i++; i < ht->capacity)
    {
        if(ht->entries[i].key)
            free(ht->entries[i].key);
    }
    free(ht->entries);
    free(ht);
}
//Use linear Search if key wasn't found at index
void *get_value(struct hash_table *ht, char *key)
{
    u_int64_t hash = hash_key(key);
    size_t index = (size_t)(hash % (u_int64_t)ht->capacity);
    struct hash_entry entry = ht->entries[index];

    while(entry.key)
    {
        if(!strcmp(entry.key, key))
            return entry.value;
        index ++;
        //Warp at end
        if(index >= ht->capacity)
            index = 0;
        entry = ht->entries[index];
    }
    return NULL;
}
//Expand capacity if no more space
char *set_value(struct hash_table *ht, char *key, void *value)
{
    if(!value)
        return NULL;

    if(ht->length > (ht->capacity / 2))
    {
        if(!expand_hash_table(ht))
            return NULL;
    }
    return set_value_entry(ht->entries, ht->capacity, key, value,
                        &ht->length);
}

char *set_value_entry(struct hash_entry *entries, size_t capacity, char *key, void *value, size_t *length)
{
    u_int64_t hash = hash_key(key);
    size_t index = (size_t)(hash % (u_int64_t)capacity);
    struct hash_entry entry = entries[index];

    while (entry.key) {
        if (!strcmp(key, entry.key)) {
            entries[index].value = value;
            return entry.key;
        }

        index++;
        if (index >= capacity)
            index = 0;
        entry = entries[index];
    }

    if(!length)
        return NULL;
    //Copy key
    if((key = strdup(key)) == NULL)
        return NULL;

    entries[index].key = key;
    entries[index].value = value;
    (*length)++;
    return key;
}
int expand_hash_table(struct hash_table *ht)
{
    int i;
    size_t n_capacity;
    struct hash_entry *n_entries;
    if(!ht)
        return 0;
    if((n_capacity = ht->capacity * 2) < ht->capacity) //Overflow
        return 0;
    
    if((n_entries = malloc(n_capacity * sizeof(struct hash_entry))) == NULL)
        return 0;

    for(i = 0; i < n_capacity; i++)
    {
        n_entries[i].key = NULL;
        n_entries->value = NULL;
    }

    //copy old values in n_entrie

    free(ht->entries);
    ht->entries = n_entries;
    ht->capacity = n_capacity;
    return 1;
}
// Return 64-bit FNV-1a hash for key (NUL-terminated). See description:
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
u_int64_t hash_key(char *key)
{
    u_int64_t hash = FNV_OFFSET;
    char *p = key;
    //This stop at NULL-terminatingh term '\0'
    while(*p)
    {
        hash ^= (u_int64_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
        p++;
    }
    return hash;
}