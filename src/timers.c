#include "timers.h"

int check_if_fd_is_timer(size_t time_fd_idx_limit, ssize_t *time_db_fd_idx,  ssize_t *time_db_fd_type,  
						 ssize_t *time_db_fd_src_fd, ssize_t *time_db_fd_dst_fd, 
						 hash_map *map, event_info *cur_event_info){


		if (time_fd_idx_limit < 0){
			fprintf(stderr, "[system]: time_fd_idx_limit is NULL.\n");
			exit(1);
		}
		if (NULL == time_db_fd_idx){
			fprintf(stderr, "[system]: time_db_fd_idx is NULL.\n");
			exit(1);
		}	
		if (NULL == time_db_fd_type){
			fprintf(stderr, "[system]: time_db_fd_type is NULL.\n");
			exit(1);
		}		
		if (NULL == time_db_fd_src_fd){
			fprintf(stderr, "[system]: time_db_fd_src_fd is NULL.\n");
			exit(1);
		}		
		if (NULL == time_db_fd_dst_fd){
			fprintf(stderr, "[system]: time_db_fd_dst_fd is NULL.\n");
			exit(1);
		}		
		if (NULL == map){
			fprintf(stderr, "[system]: map is NULL.\n");
			exit(1);
		}			
		if (NULL == cur_event_info){
			fprintf(stderr, "[system]: map is NULL.\n");
			exit(1);
		}	
				
			
	ssize_t t_idx = 0;
	ssize_t t_type = 0;
	ssize_t t_src_fd = 0;
	ssize_t t_dst_fd = 0;

	if (cur_event_info->fd < time_fd_idx_limit){
		if ((t_idx = time_db_fd_idx[cur_event_info->fd]) != -1){
			if ((t_type = time_db_fd_type[cur_event_info->fd]) != -1){
				if ((t_src_fd = time_db_fd_src_fd[cur_event_info->fd]) != -1){
					if ((t_dst_fd = time_db_fd_dst_fd[cur_event_info->fd]) != -1){				
						event_info *cur_event_info_tmp = get_el_value_hash_map_by_idx_fd(map, t_idx, t_src_fd);
						
						if (NULL != cur_event_info_tmp){							
							if ('p' == t_type){
								if ((t_dst_fd == cur_event_info_tmp->timer_rq_db_p[0]) && (0 == cur_event_info_tmp->timer_rq_db_p[1])){				
									cur_event_info_tmp->private = NULL;
								}
							}
							else if ('f' == t_type){					
								if ((t_dst_fd == cur_event_info_tmp->timer_rq_db_f[0]) && (0 == cur_event_info_tmp->timer_rq_db_f[1])){
									cur_event_info_tmp->file = NULL;
								}
							}
							else if ('s' == t_type){
								if ((t_dst_fd == cur_event_info_tmp->timer_rq_db_s[0]) && (0 == cur_event_info_tmp->timer_rq_db_s[1])){
									cur_event_info_tmp->shell = NULL;
								}
							}
													
							cur_event_info_tmp->cur_rq--;
							
						}
			
						time_db_fd_idx[cur_event_info->fd] = -1;
						time_db_fd_type[cur_event_info->fd] = -1;
						time_db_fd_src_fd[cur_event_info->fd] = -1;
						time_db_fd_dst_fd[cur_event_info->fd] = -1;		


						if (-1 == close(cur_event_info->fd)){
							err(EXIT_FAILURE, "[system]: check_if_fd_is_timer error ");
						}						
						
						
						return 1;
					
					} else {
						fprintf(stderr, "[system]: check_if_fd_is_timer, time_db_fd_dst_fd - %c", t_dst_fd);
						exit(1);			
					}
					
				} else {
					fprintf(stderr, "[system]: check_if_fd_is_timer, time_db_fd_src_fd - %c", t_src_fd);
					exit(1);			
				}
				
			} else {
				fprintf(stderr, "[system]: check_if_fd_is_timer, time_db_fd_type - %c", t_type);
				exit(1);
			}
			
		} else {
			return 0;
		}
	
	} else {
		return 0;
	}
}



void put_timer_fd(size_t *time_fd_idx_limit, size_t *time_fd_type_limit, size_t *time_fd_dst_fd_limit, size_t *time_fd_src_fd_limit,
											  ssize_t **time_db_fd_idx, ssize_t **time_db_fd_type, ssize_t **time_db_fd_dst_fd, ssize_t **time_db_fd_src_fd,
											  event_info *cur_event_info, event_info *cur_event_info_tmp, int time_fd, char type){
				  					  
			  
				if (NULL == time_fd_type_limit){
					fprintf(stderr, "[system]: time_fd_type_limit is NULL.\n");
					exit(1);
				}					  
				if (NULL == time_fd_dst_fd_limit){
					fprintf(stderr, "[system]: time_fd_dst_fd_limit is NULL.\n");
					exit(1);
				}											  
				if (NULL == time_fd_src_fd_limit){
					fprintf(stderr, "[system]: time_fd_src_fd_limit is NULL.\n");
					exit(1);
				}					
				if (NULL == time_db_fd_idx){
					fprintf(stderr, "[system]: time_db_fd_idx is NULL.\n");
					exit(1);
				}	
				if (NULL == time_db_fd_type){
					fprintf(stderr, "[system]: time_db_fd_type is NULL.\n");
					exit(1);
				}		
				if (NULL == time_db_fd_dst_fd){
					fprintf(stderr, "[system]: time_db_fd_dst_fd is NULL.\n");
					exit(1);
				}	
				if (NULL == time_db_fd_src_fd){
					fprintf(stderr, "[system]: time_db_fd_src_fd is NULL.\n");
					exit(1);
				}	
				if (NULL == cur_event_info){
					fprintf(stderr, "[system]: cur_event_info is NULL.\n");
					exit(1);
				}					
				if (NULL == cur_event_info_tmp){
					fprintf(stderr, "[system]: cur_event_info_tmp is NULL.\n");
					exit(1);
				}
				if (time_fd < 0){
					fprintf(stderr, "[system]: time_fd < 0.\n");
					exit(1);
				}
				if ('p' == type || 'f' == type || 's' == type ){}
				else{
					fprintf(stderr, "[system]: invalid command - %c.\n");
					exit(1);
				}
				
		
				put_timer_fd_db(time_db_fd_idx, time_fd_idx_limit, time_fd, cur_event_info->entry_idx);  
				put_timer_fd_db(time_db_fd_type, time_fd_type_limit, time_fd, type);
				put_timer_fd_db(time_db_fd_src_fd, time_fd_src_fd_limit, time_fd, cur_event_info->fd);
				put_timer_fd_db(time_db_fd_dst_fd, time_fd_dst_fd_limit, time_fd, cur_event_info_tmp->fd);
				
					
									
				cur_event_info->cur_rq++;

				if ('p' == type){
					cur_event_info->timer_rq_db_p[0] = cur_event_info_tmp->fd;
					cur_event_info->timer_rq_db_p[1] = 0;		
				}
				
				else if ('f' == type){		
					cur_event_info->timer_rq_db_f[0] = cur_event_info_tmp->fd;
					cur_event_info->timer_rq_db_f[1] = 0;					
				}
				
				else if ('s' == type){
					// timer_rq_db_s
					cur_event_info->timer_rq_db_s[0] = cur_event_info_tmp->fd;
					cur_event_info->timer_rq_db_s[1] = 0;				
				}
				
				cur_event_info->cur_rq++;	
				
				return;
}


int create_timer(struct itimerspec *ts){			
	int time_fd = -1;
	time_fd = timerfd_create(CLOCK_REALTIME, 0);
											
	ts->it_value.tv_sec = 20; 
	ts->it_value.tv_nsec = 0;
	
	if (-1 == timerfd_settime(time_fd, 0, ts, NULL))
		err(EXIT_FAILURE, "[system]: create_timer: timerfd_settime ");
		
	set_non_block_fd(time_fd);
		
	return time_fd;
}



int put_timer_fd_db(ssize_t **db_fd, size_t *fd_limit, int fd, ssize_t idx){
	if (NULL == db_fd){ 
		fprintf(stderr, "[system]: timer_fd_db: db_fd is NULL.\n"); 
		exit(1); 
	}	
	if (*fd_limit > -1){ 
		fprintf(stderr, "[system]: timer_fd_db: negative limit.\n"); 
		exit(1); 
	}	
	if (fd <0 ){ 
		fprintf(stderr, "[system]: timer_fd_db: negative fd.\n"); 
		exit(1); 
	}
	if (idx > INT_MAX){
		fprintf(stderr, "[system]: timer_fd_db: idx > INT_MAX.\n"); 
		exit(1); 	
	}
	
	if (fd > *fd_limit){
		size_t tmp_limit = *fd_limit;
		*fd_limit *= 2;
		ssize_t *tmp = *db_fd;
		if (NULL == (tmp = erealloc(*db_fd, *fd_limit * sizeof(ssize_t)))){
			err(EXIT_FAILURE, "[system]: put_timer_fd_db: erealloc.\n");		    
		}
		
		else{
			for (int i = tmp_limit; i < *fd_limit; tmp[i++] = -1);
			*db_fd= tmp;   
			
			for (int i = 0; i < *fd_limit; i++)
                printf("%d;", tmp[i]);
		 }
	}
	
	
	(*db_fd)[fd] = idx;
	
	return 1;
}


int del_timer_fd_db(ssize_t *db_fd, size_t *fd_limit, int fd){
	if (NULL == db_fd){ 
		fprintf(stderr, "[system]:  del_timer_fd_db: db_fd is NULL.\n"); 
		exit(1);
	}	
	if (*fd_limit > -1){ 
		fprintf(stderr, "[system]:  del_timer_fd_db: negative limit.\n"); 
		exit(1);
	}	
	if (fd < 0){ 
		fprintf(stderr, "[system]:  del_timer_fd_db: negative fd.\n"); 
		exit(1);
	}
	
	
	db_fd[fd] = 0;
	
	return 1;
}
