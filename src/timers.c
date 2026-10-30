#include "timers.h"



void init_timer_db(timers_db *timer_db){
	if (NULL == timer_db){
		fprintf(stderr, "[system]: init_timer_db, timer_db is NULL.\n");
		exit(1);
	}
	
	timer_db->idx_limit = 100;
	timer_db->type_limit = 100;
	timer_db->src_fd_limit = 100;
	timer_db->dst_fd_limit = 100;
	
	timer_db->db_fd_idx = ecalloc(timer_db->idx_limit, sizeof(ssize_t));
	for (size_t i = 0; i <timer_db->idx_limit; i++) timer_db->db_fd_idx[i] = -1;
	
	timer_db->db_fd_type = ecalloc(timer_db->type_limit, sizeof(ssize_t));
	for (size_t i = 0; i < timer_db->type_limit; i++) timer_db->db_fd_type[i] = -1;
	
	timer_db->db_fd_src = ecalloc(timer_db->src_fd_limit, sizeof(ssize_t));
	for (size_t i = 0; i < timer_db->src_fd_limit; i++) timer_db->db_fd_src[i] = -1;
	
	timer_db->db_fd_dst = ecalloc(timer_db->dst_fd_limit, sizeof(ssize_t));
	for (size_t i = 0; i < timer_db->dst_fd_limit; i++) timer_db->db_fd_dst[i] = -1;		
}	
	


int check_if_fd_is_timer(timers_db *timer_db,  hash_map *map, event_info *cur_event_info){
	if (NULL == timer_db){
		fprintf(stderr, "[system]: timer_db is NULL.\n");
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
	
	size_t tmp_fd = (size_t)cur_event_info->fd;

	if (tmp_fd < timer_db->idx_limit){
		if ((t_idx = timer_db->db_fd_idx[cur_event_info->fd]) != -1){
			if ((t_type = timer_db->db_fd_type[cur_event_info->fd]) != -1){
				if ((t_src_fd = timer_db->db_fd_src[cur_event_info->fd]) != -1){
					if ((t_dst_fd = timer_db->db_fd_dst[cur_event_info->fd]) != -1){
					 
						fprintf(stderr, "~~~~~~~~~1~~~~~~~~~~~\n");
						event_info *cur_event_info_tmp = get_el_value_hash_map_by_idx_fd(map, t_idx, t_src_fd);
						fprintf(stderr, "~~~~~~~~~2~~~~~~~~~~~\n");
						
						
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
				
			
						timer_db->db_fd_idx[cur_event_info->fd] = -1;
						timer_db->db_fd_type[cur_event_info->fd] = -1;
						timer_db->db_fd_src[cur_event_info->fd] = -1;
						timer_db->db_fd_dst[cur_event_info->fd] = -1;		


						if (-1 == close(cur_event_info->fd)){
							err(EXIT_FAILURE, "[system]: check_if_fd_is_timer error ");
						}											
						return 1;
					
					} else {
						fprintf(stderr, "[system]: check_if_fd_is_timer, time_db_fd_dst_fd - %ld", t_dst_fd);
						exit(1);			
					}
					
				} else {
					fprintf(stderr, "[system]: check_if_fd_is_timer, time_db_fd_src_fd - %ld", t_src_fd);
					exit(1);			
				}
				
			} else {
				fprintf(stderr, "[system]: check_if_fd_is_timer, time_db_fd_type - %ld", t_type);
				exit(1);
			}
			
		} else {
			return 0;
		}
	
	} else {
		return 0;
	}
}


int put_timer_fd_db(ssize_t **timer_el_db, size_t *limit, int fd, ssize_t num){
	if (NULL == timer_el_db){ 
		fprintf(stderr, "[system]: timer_fd_db: timer_el_db is NULL.\n"); 
		exit(1); 
	}		
	if (fd < 0 ){ 
		fprintf(stderr, "[system]: timer_fd_db: fd < 0.\n"); 
		exit(1); 
	}
	if (num > INT_MAX){
		fprintf(stderr, "[system]: timer_fd_db: num > INT_MAX.\n"); 
		exit(1); 	
	}
	
	size_t tmp_fd = (size_t)fd;
	size_t tmp_limit = *limit;
	
	while (*limit < tmp_fd){
		if (0 == *limit || 1 == *limit){
			*limit = 2;
		} else {
			*limit *= *limit;
		}
		if (*limit > tmp_fd){
		    break;
		}
	}

	ssize_t *tmp = *timer_el_db;
	if (NULL == (tmp = erealloc(*timer_el_db, *limit * sizeof(ssize_t)))){
		err(EXIT_FAILURE, "[system]: put_timer_fd_db: erealloc.\n");		    
	} else{
		for (size_t i = tmp_limit; i < *limit; tmp[i++] = -1);
		*timer_el_db = tmp;   	
	}
	
	(*timer_el_db)[tmp_fd] = num;

	
	return 1;
}


void put_timer_fd(timers_db *timer_db, event_info *cur_event_info, event_info *cur_event_info_tmp, int time_fd, char type){	
	if (NULL == timer_db){
		fprintf(stderr, "[system]: timer_db is NULL.\n");
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
		fprintf(stderr, "[system]: invalid command - %c.\n", type );
		exit(1);
	}		
	
	put_timer_fd_db(&timer_db->db_fd_idx, &timer_db->idx_limit, time_fd, cur_event_info->entry_idx);    	
	put_timer_fd_db(&timer_db->db_fd_type, &timer_db->type_limit, time_fd, type);
	put_timer_fd_db(&timer_db->db_fd_src, &timer_db->src_fd_limit, time_fd, cur_event_info->fd);
	put_timer_fd_db(&timer_db->db_fd_dst, &timer_db->dst_fd_limit, time_fd, cur_event_info_tmp->fd);	

							
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
		cur_event_info->timer_rq_db_s[0] = cur_event_info_tmp->fd;
		cur_event_info->timer_rq_db_s[1] = 0;				
	}
	
	cur_event_info->cur_rq++;
	
	
	return;
}


int create_timer(struct itimerspec *ts){			
	int time_fd = -1;
	time_fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
											
	ts->it_value.tv_sec = 20; 
	ts->it_value.tv_nsec = 0;
	
	if (-1 == timerfd_settime(time_fd, 0, ts, NULL))
		err(EXIT_FAILURE, "[system]: create_timer: timerfd_settime ");
		
		
	return time_fd;
}


