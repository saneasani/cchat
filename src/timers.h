#ifndef H_TIMERS
#define H_TIMERS

#include "events.h"
#include "entry_map.h"

#include <limits.h>
#include <unistd.h>
#include <sys/timerfd.h>


typedef struct{
	size_t idx_limit;
	size_t type_limit;
	size_t src_fd_limit;
	size_t dst_fd_limit;
	
	ssize_t *db_fd_idx;
	ssize_t *db_fd_type;
	ssize_t *db_fd_src;
	ssize_t *db_fd_dst;
	
} timers_db;


void init_timer_db(timers_db *timer_db);
int check_if_fd_is_timer(timers_db *timer_db,  hash_map *map, event_info *cur_event_info);
int put_timer_fd_db(ssize_t **timer_el_db, size_t *limit, int fd, ssize_t num);
void put_timer_fd(timers_db *timer_db, event_info *cur_event_info, event_info *cur_event_info_tmp, int time_fd, char type);
int create_timer(struct itimerspec *ts);
 


#endif