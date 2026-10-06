#include "entry_map.h"




#define HASHSIZE 101


ssize_t hashcode(unsigned char *s){
	if (NULL == s){
		fprintf(stderr, "[server]: hashcode: s is NULL.\n");
		exit(1);
	}
	
    ssize_t hash_val = 0;
    for(;*s;)
		hash_val = *s++ ^ hash_val + 31;
    
    return hash_val % HASHSIZE;
}


 

hash_map* create_hash_map(size_t limit, float loadFactor, float multiplier) {
		hash_map *tmp = (hash_map*)calloc(1, sizeof(hash_map));
		
		tmp->arr_size = (limit >= INITIAL_SIZE) ? limit : INITIAL_SIZE;
		tmp->data = (Entry**) calloc(tmp->arr_size, sizeof(Entry*));
		for (int i = 0; i < tmp->arr_size; tmp->data[i++] = NULL);
		
		tmp->size = 0;
		tmp->loadFactor = (loadFactor >= LOAD_FACTOR && loadFactor <= 1.0f) ? loadFactor : LOAD_FACTOR;
		tmp->limit = (int)(tmp->loadFactor * tmp->arr_size);
        tmp->multiplier = (multiplier >= MULTIPLIER) ? multiplier : MULTIPLIER;
 
        return tmp;
}

hash_map* rehash_up(hash_map **_map_, Entry* e);
int raw_put(hash_map **_map_, Entry *e);
 
 

hash_map* rehash_up(hash_map **_map_, Entry* e) {
	if (NULL ==  _map_) {
		fprintf(stderr, "[server]: rehash_up,  map is NULL.\n");
		exit(1);
	}
	if (NULL == e){
		fprintf(stderr, "[server]: rehash_up, e is NULL.\n");
		exit(1);
	}
	
	hash_map *new_map = create_hash_map((size_t)(*_map_)->arr_size * (*_map_)->multiplier,
																	 (*_map_)->loadFactor,
																	 (*_map_)->multiplier);
	size_t i = 0, size = 0;
	hash_map *map = (*_map_);
	Entry *anchor = NULL;
	Entry *target = NULL;

	size = (*_map_)->arr_size;
	
	for (i = 0; i < size; i++) {
		anchor = map->data[i];

		while (anchor) {
			target = anchor;
			anchor = anchor->next;
			raw_put(&new_map, target);
			
			free(target);
		}
		free(anchor);
	}

	free(map->data);
	free(*_map_);
	
	*_map_ = new_map;
	raw_put(&new_map, e);
	
	return new_map;
}

int raw_put(hash_map **_map_, Entry *e) {
	if (NULL ==  _map_) {
		fprintf(stderr, "[system]: raw_put map is NULL.\n");
		exit(1);
	}
	if (NULL == e){
		fprintf(stderr, "[system]: raw_put e is NULL.\n");
		exit(1);
	}
	
	hash_map *map = *_map_;
	ssize_t hash = hashcode(e->key);
	if (-1 == hash){
		fprintf(stderr, "[system]: raw_put, hash is -1.\n");
		exit(1);
	}
	
	ssize_t index = hash;

	if (map->size < map->limit) {
		if (map->data[index] == NULL) {		
			strncpy(e->value->name, e->key, 10);		
			e->value->entry_idx = hash;
			
			map->data[index] = e;
	
		} else {
			Entry *anchor = map->data[index];
			Entry *prev_user = NULL;
			
			while (anchor) {
				prev_user = anchor;
				if (!strncmp(anchor->value->name, e->key, 10)) {
					return 0;
				}
				anchor = anchor->next;
			}

			strncpy(e->value->name, e->key, 10);
			e->value->entry_idx = hash;
			
			prev_user->next = e;
			e->prev = prev_user;
		}
	} else {
		*_map_ = rehash_up(_map_, e);
	}
	(*_map_)->size++;
	
	return 1;
}


int put_el_hash_map(hash_map **map,  event_info *ptr) {
	if (NULL ==  map) {
		fprintf(stderr, "[server]: put_el_hash_map, map is NULL.\n");
		exit(1);
	}
	if (NULL == ptr){
		fprintf(stderr, "[server]: put_el_hash_map, ptr is NULL.\n");
		exit(1);
	}

	Entry *e = (Entry*)malloc(sizeof(Entry));

	strncpy(e->key, ptr->msg_chat_buf, 10);
	e->value = ptr;
	e->next = NULL;
	e->prev = NULL;
	
	int res = -1;
	if (-1 != (res = raw_put(map, e)))
		return res;
	

	free(e);
	return res;
}




int free_entry(Entry **e, control_nodes *cn) {
	if (NULL == e){
		fprintf(stderr, "[system]: free entry error, e is NULL.\n");
		exit(1);
	}
	if (NULL == e){
		fprintf(stderr, "[system]: free entry error, e is NULL.\n");
		exit(1);
	}
	
	pop_node(cn, (*e)->value);
	(*e)->next = NULL;
	(*e)->prev = NULL;
	free(*e);
	*e = NULL;
	
	return 1;
}

 
int destroy_hash_map(hash_map **_map_, control_nodes *cn) {
	hash_map *map = *_map_;
	size_t i = 0, size = 0;
	Entry *anchor = NULL;    
	Entry *target = NULL;   

	size = map->arr_size;
	 
	for (i = 0; i < size; i++) {
		anchor = map->data[i];
		
		while (anchor) {
			target = anchor;
			anchor = anchor->next;
			free_entry(&(target), cn);  
			free(target);                   
		}
		free(anchor);
	}
	 
	free(map->data);
	free(*_map_);
	*_map_ = NULL;
	
	return 0;
}


event_info *get_el_value_hash_map_by_key(hash_map *map, char *key) {

	if (NULL ==  map){
		fprintf(stderr, "[system]: get_el_value_hash_map_by_key, map is NULL.\n");
		exit(1);
	}
	if (NULL == key){
		fprintf(stderr, "[system]: get_el_value_hash_map_by_key, key is NULL.\n");
		exit(1);
	}
 
	
	ssize_t hash = hashcode(key);
	if (-1 == hash){
		fprintf(stderr, "[system]: get_el_value_hash_map_by_key, hash is -1.\n");
		exit(1);
	}
	
	size_t index = (hash % map->arr_size);
	if (-1 == index){
		fprintf(stderr, "[system]: raw_put, hash is -1.\n");
		exit(1);
	}
	
	
	if (map->data[index] != NULL) {		
		if (!strncmp(map->data[index]->key, key, 10)) {
			return map->data[index]->value;
			
		} else {
			Entry *anchor = map->data[index]->next;
			
			while (anchor) {
				if (!strncmp(anchor->key, key, 10)) {
					return map->data[index]->value;
				}
				anchor = anchor->next;
			}
		}
	}
	return NULL;
}



event_info *get_el_value_hash_map_by_idx_fd(hash_map *map, ssize_t index, int fd) {
	if (NULL ==  map){
		fprintf(stderr, "[system]: get_el_value_hash_map_by_idx_fd, map is NULL.\n");
		exit(1);
	}
	if (index < 0){
		fprintf(stderr, "[system]: get_el_value_hash_map_by_idx_fd, recevied negative index - %zu.\n", index);
		exit(1);
	}
	if (fd < 0){
		fprintf(stderr, "[system]: get_el_value_hash_map_by_idx_fd, recevied negative fd - %d.\n", fd);
		exit(1);
	}
		
	Entry *ret_val = NULL;
	
	if (map->data[index] != NULL) {
		if (map->data[index]->value->fd == fd){
			return map->data[index]->value;
			
		} else {
			Entry *anchor = map->data[index]->next;
			
			while (anchor) {
				if (anchor->value->fd == fd) {
					ret_val = anchor;
					
					break;
				}
				anchor = anchor->next;
			}
		}
	}
	if (NULL != ret_val)	
		return ret_val->value;
		
	return NULL;
}




void clear_entry_by_idx_fd(hash_map *map, ssize_t index, int fd){
	if (NULL ==  map){
		fprintf(stderr, "[system]: clear_entry_by_idx map is NULL.\n");
		exit(1);
	}
	if (index < 0){
		fprintf(stderr, "[system]: clear_entry_by_idx recevied negative index - %zu.\n", index);
		exit(1);
	}
	if (fd < 0){
		fprintf(stderr, "[system]: clear_entry_by_idx recevied negative fd - %d.\n", fd);
		exit(1);
	}
	
	Entry *tmp = NULL;

	if (map->data[index] != NULL) {
		if (map->data[index]->value->fd == fd){
			
			if (NULL != map->data[index]->next){
				map->data[index]->next->prev = NULL;
				tmp = map->data[index];
				map->data[index] = map->data[index]->next;
				
				free(tmp);
				return;
			}
			else{
				
				free(map->data[index]);
				map->data[index] = NULL;
				return;
			}
	
		} else {
			Entry *anchor = map->data[index]->next;
			
			while (anchor) {
				if (anchor->value->fd == fd) {					
					anchor->prev->next = anchor->next;
					
					if (NULL != anchor->next){
						anchor->next->prev = anchor->prev;				
					}
					
					free(anchor);
					return;
				}
				anchor = anchor->next;
			}
		}
	}
	fprintf(stderr, "[system]: clear_entry_by_idx no such index in map - %zu.\n", index);
}





void map_iterate(hash_map *map) {
	size_t size = 0, i = 0;
	size = map->arr_size;
	
	for (i = 0; i < size; i++) {
		Entry *anchor = map->data[i];
		
		while (anchor) {

			anchor = anchor->next;
		}
	}
}

