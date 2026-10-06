#ifndef H_TIMERS
#define H_TIMERS

#include "events.h"
#include "entry_map.h"
#include "common.h"

#include <limits.h>
#include <unistd.h>
#include <sys/timerfd.h>

int check_if_fd_is_timer(size_t time_fd_idx_limit, ssize_t *time_db_fd_idx,  ssize_t *time_db_fd_type,  
						 ssize_t *time_db_fd_src_fd, ssize_t *time_db_fd_dst_fd, 
						 hash_map *map, event_info *cur_event_info);
void put_timer_fd(size_t *time_fd_idx_limit, size_t *time_fd_type_limit, size_t *time_fd_dst_fd_limit, size_t *time_fd_src_fd_limit,
											  ssize_t **time_db_fd_idx, ssize_t **time_db_fd_type, ssize_t **time_db_fd_dst_fd, ssize_t **time_db_fd_src_fd,
											  event_info *cur_event_info, event_info *cur_event_info_tmp, int time_fd, char type);
int create_timer(struct itimerspec *ts);
int put_timer_fd_db(ssize_t **db_fd, size_t *fd_limit, int fd, ssize_t idx);
int del_timer_fd_db(ssize_t *db_fd, size_t *fd_limit, int fd);


#endif