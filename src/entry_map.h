#ifndef H_ENTRYMAP
#define H_ENTRYMAP

#include "events.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#define HASHSIZE 101

static const float LOAD_FACTOR = 0.5f;
static const size_t INITIAL_SIZE = HASHSIZE;
static const float MULTIPLIER = 2.0f;


typedef struct Entry_ {
        char key[10];
        event_info *value;
		struct Entry_ *next;
		struct Entry_ *prev;
		
} Entry;

typedef struct hash_map {
        Entry **data;           
        size_t size;            
        size_t arr_size;        
        size_t limit;           
                                               
        float loadFactor;                                              
        float multiplier;      

} hash_map ;


ssize_t hashcode(unsigned char *s);
hash_map* create_hash_map(size_t limit, float loadFactor, float multiplier);
hash_map* rehash_up(hash_map **_map_, Entry* e);
int raw_put(hash_map **_map_, Entry *e);
hash_map* rehash_up(hash_map **_map_, Entry* e);
int raw_put(hash_map **_map_, Entry *e);
int put_el_hash_map(hash_map **map,  event_info *ptr);
int destroy_hash_map(hash_map **_map_, control_nodes *cn);
event_info *get_el_value_hash_map_by_key(hash_map *map, char *key);
event_info *get_el_value_hash_map_by_idx_fd(hash_map *map, ssize_t index, int fd);
void clear_entry_by_idx_fd(hash_map *map, ssize_t index, int fd);
void map_iterate(hash_map *map);

#endif