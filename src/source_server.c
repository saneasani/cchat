
#include "sys_server_msgs.h"
#include "tools.h"
#include "events.h"
#include "entry_map.h"
#include "common.h"
#include "connections.h"
#include "timers.h"


#include <err.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <inttypes.h>

#define MAX_EVENTS 1000

 


int main(int argc, char **argv){
	if (argc != 2){
		fprintf(stderr, "Usage: <Program name> <Port number>.\n");
		exit(EXIT_FAILURE);
	}
	
	// --- sockets init ---
	int	listen_fd = create_tcp_listen_socket(argv[1]), conn_fd = -1;
	struct sockaddr_storage cli_addr = {0};
	socklen_t cli_addr_len = 0;
	
	
	// --- client info ---
	hash_map *map = create_hash_map(2, 0.5f, 2.0f);
	
	control_blocks *ctrl_bl_head = NULL;
	control_nodes *ctrl_nds = create_cn();


	// --- init epoll ---							
	struct epoll_event total_events[MAX_EVENTS] = {0};
	int epoll_fd = create_epoll();
	
	set_non_block_fd(listen_fd);
	add_event(epoll_fd, listen_fd, ctrl_nds, &ctrl_bl_head, 1);
	

	
	// --- global chat db ---
	size_t gl_total_users = 0;
	
	size_t gl_fd_limit = 100;
	ssize_t *db_gl_chat_fd = ecalloc(gl_fd_limit, sizeof(ssize_t));
	for (size_t i = 0; i <gl_fd_limit; i++) db_gl_chat_fd[i] = -1;
	
	size_t gl_idx_limit = gl_fd_limit/2;
	size_t gl_idx = 0;	
	ssize_t *db_gl_chat_idx = ecalloc(gl_idx_limit, sizeof(ssize_t));
	for (size_t i = 0; i < gl_idx_limit; i++) db_gl_chat_idx[i] = -1;
	


	// --- timer db---
	int time_fd = -1;
	size_t time_fd_idx = 0;

	size_t time_fd_idx_limit = 100;
	size_t time_fd_type_limit = 100;
	size_t time_fd_src_fd_limit = 100;
	size_t time_fd_dst_fd_limit = 100;
	
	struct itimerspec ts = {0};
	ts.it_value.tv_sec = 10;
	ts.it_value.tv_nsec = 0;


	ssize_t *time_db_fd_idx = ecalloc(time_fd_idx_limit, sizeof(ssize_t));
	for (size_t i = 0; i < time_fd_idx_limit; i++) time_db_fd_idx[i] = -1;

	ssize_t *time_db_fd_type = ecalloc(time_fd_type_limit, sizeof(ssize_t));
	for (size_t i = 0; i < time_fd_type_limit; i++) time_db_fd_type[i] = -1;
	
	ssize_t *time_db_fd_src_fd = ecalloc(time_fd_src_fd_limit, sizeof(ssize_t));
	for (size_t i = 0; i < time_fd_src_fd_limit; i++) time_db_fd_src_fd[i] = -1;
	
	ssize_t *time_db_fd_dst_fd = ecalloc(time_fd_dst_fd_limit, sizeof(ssize_t));
	for (size_t i = 0; i < time_fd_dst_fd_limit; i++) time_db_fd_dst_fd[i] = -1;
		
	
	// --- common ---
	char *sstm_msg = NULL;
	size_t sstm_msg_len = 0;
	
	event_info *cur_event_info = NULL;
	event_info *cur_event_info_tmp = NULL;
	
	ssize_t n_read = 0, cur_read = 0;
	int num_fds = -1, n = 0;

	
	for (;;) {	
		if (-1 == (num_fds = epoll_wait(epoll_fd, total_events, MAX_EVENTS, -1))){
			if (-1 == num_fds){
				if (EINTR == errno)	
					continue;
				err(EXIT_FAILURE, "[system]: epoll_wait error: ");
				}		
		}
		fprintf(stderr, "[system]: epoll wait ready fds: %d\n", num_fds);
		
		for (n = 0; n < num_fds; n++){
			cur_event_info = (event_info*)total_events[n].data.ptr; 
			
			fprintf(stderr, "[system]: cur_event_info->fd: %d\n", cur_event_info->fd);
		
			if (total_events[n].events & (EPOLLERR| EPOLLHUP)){
				fprintf(stderr, "[system]: epoll error.\n"); 
				
				int err_num = 0;
				socklen_t err_len = sizeof(err_num);
				
				if (getsockopt(cur_event_info->fd, SOL_SOCKET, SO_ERROR, (void *)&err_num, &err_len) == 0){
					fprintf(stderr, "[system]: error = %s.\n", strerror(err_num));
				}
			
				clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
						     ctrl_nds, &cur_event_info,  map);
				continue;
			}
			
			else if (cur_event_info->fd == listen_fd){
				for (;;){	
					cli_addr_len = sizeof(cli_addr);
					
					if (-1 == (conn_fd = accept(listen_fd, (struct sockaddr*)&cli_addr, &cli_addr_len))){
						if (EAGAIN == errno || EWOULDBLOCK == errno ||
							EPROTO == errno || ECONNABORTED == errno){
							break;	
						}
						else if (EINTR == errno){
							continue;
						}
						err(EXIT_FAILURE, "accept error: ");			
					}
					set_non_block_fd(conn_fd);
					add_event(epoll_fd, conn_fd, ctrl_nds, &ctrl_bl_head, 1);
					cur_event_info_tmp = get_head(ctrl_nds);
					
					put_el_db_num(&db_gl_chat_fd, &db_gl_chat_idx, &gl_fd_limit, &gl_idx_limit, &gl_idx, cur_event_info_tmp->fd);			
					gl_total_users++;
		
					sstm_msg = get_server_system_msg(5, NULL, 0, -1, &sstm_msg_len);
					
					if (-1 == write_server_msg(cur_event_info_tmp->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
						clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
						             ctrl_nds, &cur_event_info,  map);
					}
					
					fprintf(stderr, "[system]: new client -> %s.\n", sock_ntop((struct sockaddr*)&cli_addr, cli_addr_len));	
				} 
			}
		
			else if (total_events[n].events & EPOLLIN | EPOLLET){
				
				if (1 == check_if_fd_is_timer(time_fd_idx_limit, time_db_fd_idx, time_db_fd_type, time_db_fd_src_fd,
											   time_db_fd_dst_fd, map, cur_event_info)){
					continue;
					
				}
				
				int result = 0;
				
				for (;;){
					char buf[CHUNK_SIZE] = {0};	
					
					if (-1 == result)
						break;
						
					if (-1 == (n_read = read(cur_event_info->fd, buf, CHUNK_SIZE))){
						if (EAGAIN == errno || EWOULDBLOCK == errno){ 
							printf("[server]: Finished reading data from client - %ld\n", n_read);
							break;
						}
						else if (EINTR == errno){
							continue;	
						}
						else if (ECONNRESET == errno) { 
							clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
								 ctrl_nds, &cur_event_info,  map);			
							break; 
						} 
						err(EXIT_FAILURE, "[system]: read client error ");
					}
					printf("[server]: n_read =>:%ld\n", n_read);
					
					if  (0 == n_read){
						clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
								 ctrl_nds, &cur_event_info,  map);			 
						break;
					}
					
					printf("[server]: read raw message:\n");	
					for(int i=0;i<n_read;i++){
						fprintf(stderr, "%c ", buf[i]);	
					}
					fprintf(stderr, "\n");
					
					
					cur_read = 0;
					
					while ((n_read - cur_read) > 0){
						result = read_incoming_data(&n_read, &cur_read,  buf, cur_event_info);
						
						fprintf(stderr, "[system]: n_read => %d, cur_read =>: %d, result =>: %d\n", n_read, cur_read, result);
						fprintf(stderr, "[system]: type_msg => %d, notif_rcv =>: %d.\n", cur_event_info->type_msg, cur_event_info->notif_rcv);
									
						if (!result){
							continue;
						}
							 
						if (-1 == result){
							clear_msg_buf(cur_event_info);							
							break;
						}
					
						if (1 == result){
							fprintf(stderr, "[system]: Msg from buffer from fd: %d.\n", cur_event_info->fd);
							for(int i=0;i<n_read;i++){
								fprintf(stderr, "%c ", buf[i]);	
							}

							if (!cur_event_info->init_name){
								fprintf(stderr, "[system]: No init name for fd: %d.\n", cur_event_info->fd);
						
								int res_name = 0;
								if (-1 == (res_name = parse_name(cur_event_info->msg_chat_buf, cur_event_info->msg_chat_idx))){
									fprintf(stderr, "[system]: Error in spelling of the name. Fd: %d.\n", cur_event_info->fd);
									sstm_msg = get_server_system_msg(1, NULL, 0, -1, &sstm_msg_len);
									
									if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
											clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
														 ctrl_nds, &cur_event_info,  map);
									}
								}
								
								else{	
									int res = 0;
									if (-1 == (res = put_el_hash_map(&map, cur_event_info))){
										fprintf(stderr, "[system]: Error in db name saving. Fd: %d.\n", cur_event_info->fd);
										sstm_msg = get_server_system_msg(7, NULL, 0, -1, &sstm_msg_len);
										write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's');
									
										clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
										 ctrl_nds, &cur_event_info,  map);
										 									
										result = -1;
														
										break;

									} else if (0 == res){
										fprintf(stderr, "[system]: Name is already taken. Fd: %d.\n", cur_event_info->fd);
										sstm_msg = get_server_system_msg(9, NULL, 0, -1, &sstm_msg_len);
										
										if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
															 ctrl_nds, &cur_event_info,  map);
										}
														
										clear_msg_buf(cur_event_info);		
										result = -1;
															
										break;
									
									}
									
									cur_event_info_tmp = get_el_value_hash_map_by_key(map, cur_event_info->msg_chat_buf);
									if (NULL != cur_event_info_tmp){							
										fprintf(stderr, "[system]: Name is inserted in db. Fd: %d; name: %s.\n", cur_event_info->fd,  cur_event_info->name);				 									
										cur_event_info->init_name = 1;	
										
										sstm_msg = get_server_system_msg(6, cur_event_info->msg_chat_buf, cur_event_info->msg_chat_idx, -1, &sstm_msg_len);
										
										
						
										if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
															 ctrl_nds, &cur_event_info,  map);
										}							
									}
									else {
										fprintf(stderr, "[system]: Something wrong with inserting name in db. Fd: %d.\n", cur_event_info->fd);
										exit(1);
									}
								}
							}
							
							else if (cur_event_info->init_name){
								fprintf(stderr, "[system]: Message from: %s: \n", cur_event_info->name);	
								
								for (int i = 0; i < cur_event_info->msg_chat_idx; printf("%c", cur_event_info->msg_chat_buf[i++]));
							
								if ('[' == cur_event_info->msg_chat_buf[0] 
									&& 2 != cur_event_info->type_msg 
									&& 3 != cur_event_info->type_msg){
									
									fprintf(stderr, "[system]: Message from: %s starts with '[': \n", cur_event_info->name);	
									sstm_msg = get_server_system_msg(4, NULL, 0, -1, &sstm_msg_len);
									if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
										clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
													 ctrl_nds, &cur_event_info,  map);
									}									
								}
														
								else if ('/' == cur_event_info->msg_chat_buf[0] 
										&& 2 != cur_event_info->type_msg 
										&& 3 != cur_event_info->type_msg){
												
									char *ptr_name = NULL;
									int status = -1;
									 
								
									char c = parse_command(cur_event_info->fd, cur_event_info->msg_chat_buf, cur_event_info->msg_chat_idx, &ptr_name, &status, NULL, NULL, NULL, NULL);
									
									fprintf(stderr, "[system]: Command message from: %s; command: %c, status: %d: \n", cur_event_info->name, c, status);
									if (NULL != ptr_name){
										fprintf(stderr, "[system]: ptr_name: %s.\n", ptr_name);
									
									}
																	
									if ('o' == c){	// /online
										fprintf(stderr, "[system]: Username: %s checking online.\n", cur_event_info->name);
										sstm_msg = get_server_system_msg(0, NULL, get_size_num(gl_total_users), gl_total_users, &sstm_msg_len);
										if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
											clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
														 ctrl_nds, &cur_event_info,  map);
										}										
									}	
									
									else if ('g' == c){	// /global
										if (NULL != cur_event_info->private){
											fprintf(stderr, "[system]: Username: %s switched to global.\n", cur_event_info->name);	
											cur_event_info->private = NULL;
											cur_event_info->init_pr = 0;
																			
											
											if (-1 != cur_event_info->timer_rq_db_p[0]){					
												cur_event_info->timer_rq_db_p[0] = -1;
												cur_event_info->timer_rq_db_p[1] = -1;									
												cur_event_info->cur_rq--;	
											}		
										}
										else {
											fprintf(stderr, "[system]: Username: %s is in global.\n", cur_event_info->name);	
											sstm_msg = get_server_system_msg(2, NULL, 0, -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
															 ctrl_nds, &cur_event_info,  map);
											}											
										}
									} 
									
									else if ('p' == c && 1 == status){ // /private/username   
										fprintf(stderr, "[system]: Private request from: %s; to: %s.\n", cur_event_info->name,  ptr_name);									
										cur_event_info_tmp = NULL;
													
										if (cur_event_info->cur_rq == cur_event_info->limit_rq){									
											sstm_msg = get_server_system_msg(8, NULL, 0, -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
															 ctrl_nds, &cur_event_info,  map);									
											}									
											fprintf(stderr, "[system]: Private request. Username: %s reached his requests limits.\n", cur_event_info->name);	
										}
										
										else if (!strncmp(cur_event_info->name, ptr_name, 10)){
											sstm_msg = get_server_system_msg(11, NULL, 0, -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
													clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																 ctrl_nds, &cur_event_info,  map);								
											}
											
											fprintf(stderr, "[system]: Private request. Usernames are equal: %s : %s.\n", cur_event_info->name, ptr_name);																
										}
										
										else {								
											if (NULL == (cur_event_info_tmp = get_el_value_hash_map_by_key(map, ptr_name))){		
												sstm_msg = get_server_system_msg(3, ptr_name, strlen(ptr_name), -1, &sstm_msg_len);
												
												if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
													clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																 ctrl_nds, &cur_event_info,  map);
												}												

												fprintf(stderr, "[system]: Private request from username: %s; no such username online: %s.\n", cur_event_info->name, ptr_name);													
											}
											
											else {					
												cur_event_info->private = cur_event_info_tmp;
												cur_event_info->init_pr = 0;
																	
												time_fd = create_timer(&cur_event_info->ts);
												add_event(epoll_fd, time_fd, ctrl_nds, &ctrl_bl_head, 1);	
												put_timer_fd(&time_fd_idx_limit, &time_fd_type_limit, &time_fd_dst_fd_limit, &time_fd_src_fd_limit,
															 &time_db_fd_idx, &time_db_fd_type, &time_db_fd_dst_fd, &time_db_fd_src_fd,
															 cur_event_info, cur_event_info_tmp, time_fd, 'p');											
														
																							
												sstm_msg = get_server_system_msg(12, cur_event_info->name, strlen(cur_event_info->name), -1, &sstm_msg_len);
												
												if (-1 == write_server_msg(cur_event_info_tmp->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
													clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																 ctrl_nds, &cur_event_info,  map);
												}												
												
												fprintf(stderr, "[system]: Private request from: %s; to: %s. - sent.\n", cur_event_info->name,  ptr_name);	
											}
										}
									}	
									
									else if ('p' == c && 0 == status){ // /private/ok/username   
										fprintf(stderr, "[system]: Private ok request from: %s; to: %s.\n", cur_event_info->name,  ptr_name);				
										cur_event_info_tmp = NULL;
																
										if (NULL == (cur_event_info_tmp = get_el_value_hash_map_by_key(map, ptr_name))){
											sstm_msg = get_server_system_msg(3, ptr_name, strlen(ptr_name), -1, &sstm_msg_len);
											
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
															 ctrl_nds, &cur_event_info,  map);									
											}
											
											fprintf(stderr, "[system]: Private ok request from username: %s; no such username online: %s.\n", cur_event_info->name, ptr_name);																	
										}
										
										else if (!strncmp(cur_event_info->name, ptr_name, 10)){
											sstm_msg = get_server_system_msg(11, NULL, 0, -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
															 ctrl_nds, &cur_event_info,  map);									
											}
											
											fprintf(stderr, "[system]: Private ok request. Usernames are equal: %s : %s.\n", cur_event_info->name, ptr_name);																				
										}	
										
										else {								
											if (cur_event_info_tmp->private == cur_event_info){												
												cur_event_info->private = cur_event_info_tmp;
												
												cur_event_info->init_pr = 1;
												cur_event_info_tmp->init_pr = 1;									
												cur_event_info_tmp->cur_rq--;									
												cur_event_info_tmp->timer_rq_db_p[1] = 1;
									
														
												// src
												sstm_msg = get_server_system_msg(14, cur_event_info->name, strlen(cur_event_info->name) , -1, &sstm_msg_len);
												if (-1 == write_server_msg(cur_event_info_tmp->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
													clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																 ctrl_nds, &cur_event_info,  map);				
												}								
												// dst
												sstm_msg = get_server_system_msg(13, ptr_name, strlen(ptr_name), -1, &sstm_msg_len);
												if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
													clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																 ctrl_nds, &cur_event_info,  map);											
												}
												
												fprintf(stderr, "[system]: Private ok request from: %s; to: %s. - confirmed.\n", cur_event_info->name,  ptr_name);	
											}	
											
											else{
												sstm_msg = get_server_system_msg(15, ptr_name, strlen(ptr_name), -1, &sstm_msg_len);
												if ( -1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
													clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																 ctrl_nds, &cur_event_info,  map);										
												}

												fprintf(stderr, "[system]: Private request from: %s; to: %s. - no such request or time elapsed.\n", cur_event_info->name,  ptr_name);
											}
										}
									}										
			
									else if ('f' == c && 1 == status){ // /file/username  
										fprintf(stderr, "[system]: File send request from: %s; to: %s.\n", cur_event_info->name,  ptr_name);
										cur_event_info_tmp = NULL;
										
			
										if (cur_event_info->cur_rq == cur_event_info->limit_rq){
											sstm_msg = get_server_system_msg(8, NULL, 0, -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
															 ctrl_nds, &cur_event_info,  map);										
											}
											
											fprintf(stderr, "[system]: File send request. Username: %s reached his requests limits.\n", cur_event_info->name);
										}
										
										else if (!strncmp(cur_event_info->name, ptr_name, 10)){
											sstm_msg = get_server_system_msg(11, NULL, 0, -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
															 ctrl_nds, &cur_event_info,  map);											
											}
											
											fprintf(stderr, "[system]: File send request. Usernames are equal: %s : %s.\n", cur_event_info->name, ptr_name);																		
										}																		
							
										else {
											if (NULL == (cur_event_info_tmp = get_el_value_hash_map_by_key(map, ptr_name))){
												sstm_msg = get_server_system_msg(3, ptr_name, strlen(ptr_name), -1, &sstm_msg_len);
												if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
													clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																 ctrl_nds, &cur_event_info,  map);																					
												}
												
												fprintf(stderr, "[system]: File send request from username: %s; no such username online: %s.\n", cur_event_info->name, ptr_name);									
											}	
											
											else {	 			
												if (NULL != cur_event_info->file){
													sstm_msg = get_server_system_msg(25, NULL, 0, -1, &sstm_msg_len);
													if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
															 ctrl_nds, &cur_event_info,  map);										
													}
												
												} else {
													cur_event_info->file = cur_event_info_tmp;
																											
													time_fd = create_timer(&cur_event_info->ts);	
													add_event(epoll_fd, time_fd, ctrl_nds, &ctrl_bl_head, 1);	
														
													put_timer_fd(&time_fd_idx_limit, &time_fd_type_limit, &time_fd_dst_fd_limit, &time_fd_src_fd_limit,
																 &time_db_fd_idx, &time_db_fd_type, &time_db_fd_dst_fd, &time_db_fd_src_fd,
																 cur_event_info, cur_event_info_tmp, time_fd, 'f');											
															
													
													sstm_msg = get_server_system_msg(17, cur_event_info->name, strlen(cur_event_info->name), -1, &sstm_msg_len);
													
													if (-1 == write_server_msg(cur_event_info_tmp->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																	 ctrl_nds, &cur_event_info,  map);																									
													}

													fprintf(stderr, "[system]: File send request from: %s; to: %s. - sent.\n", cur_event_info->name,  ptr_name);	
												}
											}
										}
									}		
									
									else if ('f' == c && 0 == status){ // /file/ok/username  
										fprintf(stderr, "[system]: File send ok request from: %s; to: %s.\n", cur_event_info->name,  ptr_name);						
										cur_event_info_tmp = NULL;
										
										if (NULL == (cur_event_info_tmp = get_el_value_hash_map_by_key(map, ptr_name))){
											sstm_msg = get_server_system_msg(3, ptr_name, strlen(ptr_name), -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
															 ctrl_nds, &cur_event_info,  map);									
											}

											fprintf(stderr, "[system]: File send ok request from username: %s; no such username online: %s.\n", cur_event_info->name, ptr_name);				
										}
										
										else if (!strncmp(cur_event_info->name, ptr_name, 10)){
											sstm_msg = get_server_system_msg(11, NULL, 0, -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
															 ctrl_nds, &cur_event_info,  map);											
											}
											
											fprintf(stderr, "[system]: File send ok request. Usernames are equal: %s : %s.\n", cur_event_info->name, ptr_name);																				
										}																			
										
										else{	
											if (NULL != cur_event_info->file){
												sstm_msg = get_server_system_msg(25, NULL, 0, -1, &sstm_msg_len);
												if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
													clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																 ctrl_nds, &cur_event_info,  map);												
												}
												
											} else {
												if (cur_event_info_tmp->file == cur_event_info){
													cur_event_info->file = cur_event_info_tmp;														 
													cur_event_info_tmp->cur_rq--;									
													cur_event_info_tmp->timer_rq_db_f[1] = 1;
													cur_event_info_tmp->fl_file_src_init = 1;
																						
													// src
													sstm_msg = get_server_system_msg(19, ptr_name, strlen(ptr_name), -1, &sstm_msg_len);
													if (-1 == write_server_msg(cur_event_info_tmp->fd, sstm_msg, sstm_msg_len, 1, 1, 's')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																	 ctrl_nds, &cur_event_info,  map);																						
													}
																								
													// dst
													sstm_msg = get_server_system_msg(18, cur_event_info->name, strlen(cur_event_info->name), -1, &sstm_msg_len);
													if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 2, 's')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																	 ctrl_nds, &cur_event_info,  map);																							
													}
													
													fprintf(stderr, "[system]: File send ok request from: %s; to: %s. - confirmed.\n", cur_event_info->name,  ptr_name);	
												}							
												else{
													sstm_msg = get_server_system_msg(20, NULL, 0, -1, &sstm_msg_len);
													if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){													
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																	 ctrl_nds, &cur_event_info,  map);																										
													}
												 										
													fprintf(stderr, "[system]: File send ok request from: %s; to: %s. - no such request or time elapsed.\n", cur_event_info->name,  ptr_name);
												}
											}
										}			
									}
									
									else if ('s' == c && 1 == status){ 	// /shell/username		
										fprintf(stderr, "[system]: Shell manage request from: %s; to: %s. - sent.\n", cur_event_info->name,  ptr_name);
										cur_event_info_tmp = NULL;
									
										if (cur_event_info->cur_rq == cur_event_info->limit_rq){
											sstm_msg = get_server_system_msg(8, NULL, 0, -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																	 ctrl_nds, &cur_event_info,  map);	
											}
											
											fprintf(stderr, "[system]: Shell manage request. Username: %s reached his requests limits.\n", cur_event_info->name);	
										}
										
										else if (!strncmp(cur_event_info->name, ptr_name, 10)){
											sstm_msg = get_server_system_msg(11, NULL, 0, -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																	 ctrl_nds, &cur_event_info,  map);
											}
											
											fprintf(stderr, "[system]: Shell manage request. Usernames are equal: %s : %s.\n", cur_event_info->name, ptr_name);											
										}																						
										
										else {						
											if (NULL == (cur_event_info_tmp = get_el_value_hash_map_by_key(map, ptr_name))){
												sstm_msg = get_server_system_msg(3, ptr_name, strlen(ptr_name), -1, &sstm_msg_len);
												if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
													clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																	 ctrl_nds, &cur_event_info,  map);
												}

												fprintf(stderr, "[system]: Shell manage request from username: %s; no such username online: %s.\n", cur_event_info->name, ptr_name);
												
											}else {									
												if (NULL != cur_event_info->shell){
													sstm_msg = get_server_system_msg(32, NULL, 0, -1, &sstm_msg_len);
													if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																														 ctrl_nds, &cur_event_info,  map);	
													}
													
												} else {		
													cur_event_info->shell = cur_event_info_tmp;
													
													time_fd = create_timer(&cur_event_info->ts);
													add_event(epoll_fd, time_fd, ctrl_nds, &ctrl_bl_head, 1);																
													put_timer_fd(&time_fd_idx_limit, &time_fd_type_limit, &time_fd_dst_fd_limit, &time_fd_src_fd_limit,
																 &time_db_fd_idx, &time_db_fd_type, &time_db_fd_dst_fd, &time_db_fd_src_fd,
																 cur_event_info, cur_event_info_tmp, time_fd, 's');		


													sstm_msg = get_server_system_msg(26, cur_event_info->name, strlen(cur_event_info->name), -1, &sstm_msg_len);
													if (-1 == write_server_msg(cur_event_info_tmp->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																														 ctrl_nds, &cur_event_info,  map);
													}													

													fprintf(stderr, "[system]: Shell manage request from: %s; to: %s. - sent.\n", cur_event_info->name,  ptr_name);	
												}
											}
										}
									
									} 
									
									else if ('s' == c && 0 == status){	// /shell/ok/username  
										fprintf(stderr, "[system]: Shell manage ok request from: %s; to: %s.\n", cur_event_info->name,  ptr_name);										
										cur_event_info_tmp = NULL;
										
										if (NULL == (cur_event_info_tmp = get_el_value_hash_map_by_key(map, ptr_name))){
											sstm_msg = get_server_system_msg(3, ptr_name, strlen(ptr_name), -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
											clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
														ctrl_nds, &cur_event_info, map);
											}											
											
											fprintf(stderr, "[system]: Shell manage ok request from username: %s; no such username online: %s.\n", cur_event_info->name, ptr_name);
										}
										
										else if (!strncmp(cur_event_info->name, ptr_name, 10)){
											sstm_msg = get_server_system_msg(11, NULL, 0, -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
															 ctrl_nds, &cur_event_info,  map);
											}	
											
											fprintf(stderr, "[system]: Shell manage ok request. Usernames are equal: %s : %s.\n", cur_event_info->name, ptr_name);																
										}										
																	
										else{	 	
											if (NULL != cur_event_info->shell){
												sstm_msg = get_server_system_msg(32, NULL, 0, -1, &sstm_msg_len);
												if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																	 ctrl_nds, &cur_event_info,  map);
												}									
											} else {								
					
												if (cur_event_info_tmp->shell == cur_event_info){
													cur_event_info->shell = cur_event_info_tmp;
													cur_event_info->init_dst_shell = 1;
													cur_event_info_tmp->init_src_shell = 1;										
													cur_event_info_tmp->timer_rq_db_s[1] = 1;	
													
									
													// src
													sstm_msg = get_server_system_msg(27, cur_event_info->name, strlen(cur_event_info->name), -1, &sstm_msg_len);
													if (-1 == write_server_msg(cur_event_info_tmp->fd, sstm_msg, sstm_msg_len, 1, 3, 's')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																	 ctrl_nds, &cur_event_info,  map);
													}		
													
													// dst
													sstm_msg = get_server_system_msg(28, cur_event_info_tmp->name, strlen(cur_event_info_tmp->name), -1, &sstm_msg_len);
													if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 4, 's')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																	 ctrl_nds, &cur_event_info,  map);
													}		

													fprintf(stderr, "[system]: Shell manage ok request from: %s; to: %s. - confirmed.\n", cur_event_info->name,  ptr_name);
												} 
												
												else {
													sstm_msg = get_server_system_msg(29, NULL, 0, -1, &sstm_msg_len);
													if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 4, 's')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																	 ctrl_nds, &cur_event_info,  map);
													}	

													fprintf(stderr, "[system]: Shell manage ok request from: %s; to: %s. - sent.\n", cur_event_info->name,  ptr_name);												
												}
											}
										}												
									}
									
									if (NULL != ptr_name)
										memset(ptr_name, 0, 9);
							
								} else { 
									if (1 == cur_event_info->type_msg){		
										fprintf(stderr, "[system]: msg type = 1.\n");	
										if (NULL != cur_event_info->private){
																			
											if (NULL != cur_event_info->private->private){ 
												if (cur_event_info->private->private->fd == cur_event_info->fd){
													if (-1 == write_private_msg(cur_event_info)){
															clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																		 ctrl_nds, &cur_event_info,  map);
														}												
													fprintf(stderr, "[system]: Private message from: %s; to: %s: \n", cur_event_info->name, cur_event_info->private->private->name);		
												
												} else {
													if (1 == cur_event_info->init_pr){								
														sstm_msg = get_server_system_msg(15, cur_event_info->private->name, strlen(cur_event_info->private->name), -1, &sstm_msg_len);
														if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
															clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																		 ctrl_nds, &cur_event_info,  map);
														}			
														fprintf(stderr, "[system]: Private from: %s: but user has left the chat: %s: \n", cur_event_info->name, cur_event_info->private->private->name);
													}
													
													else if (0 == cur_event_info->init_pr){
														write_global_msg(cur_event_info, db_gl_chat_idx, gl_idx);
													}
												}
											}
											
											else { 
												if (1 == cur_event_info->init_pr){ 
													sstm_msg = get_server_system_msg(16, NULL, 0, -1, &sstm_msg_len);
													if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
															clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																		 ctrl_nds, &cur_event_info,  map);
													}
		
													fprintf(stderr, "[system]: Private from: %s: but user has left the chat.\n", cur_event_info->name);
												}
												
												else if (0 == cur_event_info->init_pr){
													write_global_msg(cur_event_info, db_gl_chat_idx, gl_idx);													
												}
											}											
										} 
										
										else {  
											write_global_msg(cur_event_info, db_gl_chat_idx, gl_idx);												
										}
									}
									
									else if (2 == cur_event_info->type_msg){ 
										fprintf(stderr, "[system]: msg type = 2.\n");	
									
										if (1 == cur_event_info->fl_file_src_init){
											cur_event_info_tmp = cur_event_info->file;	
												
											if (0 == cur_event_info->fl_file_name){
												cur_event_info->fl_file_name = 1;										
												strncpy(cur_event_info->file_name, cur_event_info->msg_chat_buf, MAX_LINE);
												
												if (-1 == write_server_msg(cur_event_info_tmp->fd,  cur_event_info->file_name, cur_event_info->len_chat_msg, 2, 0, 's')){
													clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																 ctrl_nds, &cur_event_info,  map);
												}	
												
												fprintf(stderr, "[system]: Recevied filename from %s: filename: %s: \n", cur_event_info->name, cur_event_info->file_name);	
											}
											
											else if (0 == cur_event_info->fl_file_size) {
												cur_event_info->fl_file_size = 1;											
												
												cur_event_info->left_file_size = cur_event_info->num_data;
												if (-1 == write_server_msg(cur_event_info_tmp->fd,  &cur_event_info->num_data, 0, 2, 0, 'n')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																	 ctrl_nds, &cur_event_info,  map);
													}	
																	
												fprintf(stderr, "[system]: Recevied filesize from %s: filesize: %" PRIu32 ": \n", cur_event_info->name,  cur_event_info->num_data);	
																						
												static char edited_file_name[50];
												snprintf(edited_file_name, sizeof(edited_file_name), "[system][server] %s", cur_event_info->file_name);
										
											
												while (1) {
													if (-1 == (cur_event_info->file_read_fd = open(edited_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644))){
						
														if (EINTR == errno){
															continue;
														}
														else if (EACCES == errno){
															fprintf(stderr, "[system]: You don't have acces of the file.\n");
															break;
														}
														else if (ENOTDIR == errno){
															fprintf(stderr, "[system]: The file is directory.\n");															
															break;
														}
														else if (ENOENT == errno){
															fprintf(stderr, "[system]: The file doesn't exists.\n");																
															break;
														}
														err(EXIT_FAILURE, "[system]: Error user file, ");
													}
													break;
												}							
											}	
											
											 else if (0 == cur_event_info->fl_file_chunks){
												if (cur_event_info->left_file_size > 0){
													fprintf(stderr, "[system]: File send chunks. Write to file from: %s: msg: %s.\n", cur_event_info->name,  cur_event_info->msg_chat_buf);
												
													if (-1 == write(cur_event_info->file_read_fd, cur_event_info->msg_chat_buf, cur_event_info->msg_chat_idx))
														err(EXIT_FAILURE, "[system]: write file chunks error, ");
												
													cur_event_info->file_offset += cur_event_info->msg_chat_idx;
													cur_event_info->left_file_size -= cur_event_info->msg_chat_idx;
													
													if (-1 == write_server_msg(cur_event_info_tmp->fd,  cur_event_info->msg_chat_buf, cur_event_info->msg_chat_idx, 2, 0, 's')){
															clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																		 ctrl_nds, &cur_event_info,  map);
													}							
	
													fprintf(stderr, "[system]: File chunk sent from %s. Left file size: %d.\n", cur_event_info->name, cur_event_info->left_file_size);										
																												
													if (cur_event_info->left_file_size == 0){		
														sstm_msg = get_server_system_msg(24, cur_event_info->file->name, strlen(cur_event_info->file->name), -1, &sstm_msg_len);
														if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
															clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																		 ctrl_nds, &cur_event_info,  map);
														}	
									
														fprintf(stderr, "[system]: All file sent. From user: %s to user: %s\n", cur_event_info->name, cur_event_info->file->name);	

														clear_file_send(cur_event_info, 'd', 's');
													}											
												}
											}	
										}
										
										else {
											sstm_msg = get_server_system_msg(22, NULL, 0, -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
													clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																 ctrl_nds, &cur_event_info,  map);
											}	
											
											fprintf(stderr, "[system]: File send is not active from: %s.\n", cur_event_info->name);					
										}			
									}
									
									else if (3 == cur_event_info->type_msg){ 
										fprintf(stderr, "[system]: msg type = 3.\n");	
									
										if (NULL != cur_event_info->shell){					 				
											if (NULL != cur_event_info->shell->shell){
												if (cur_event_info->shell->shell->fd == cur_event_info->fd){																							
													fprintf(stderr, "[system]: Shell write from: %s to: %s.\n", cur_event_info->name, cur_event_info->shell->name );	
													fprintf(stderr, "[system]: cur_event_info->init_src_shell: %d.\n", cur_event_info->init_src_shell);	
																																					
													if (!(strncmp(cur_event_info->msg_chat_buf, "exit\n", 5))){																										
														if (1 == cur_event_info->init_src_shell){
															if (-1 == write_server_msg(cur_event_info->shell->fd, cur_event_info->msg_chat_buf, cur_event_info->msg_chat_idx, 3, 2, 's')){
																clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																			 ctrl_nds, &cur_event_info,  map);								
															}
															fprintf(stderr, "[system]: Src username: %s exiting shell managing.\n", cur_event_info->name);	
															cur_event_info->init_src_shell = 0;
															cur_event_info->shell->init_dst_shell = 0;
															
															sstm_msg = get_server_system_msg(31, cur_event_info->name, strlen(cur_event_info->name), -1, &sstm_msg_len);
															if (-1 == write_server_msg(cur_event_info->shell->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
																clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																			 ctrl_nds, &cur_event_info,  map);
															}																													
														}
														
														else if (1 == cur_event_info->init_dst_shell){
															fprintf(stderr, "[system]: Dst username: %s exiting shell managing.\n", cur_event_info->name);	
															cur_event_info->init_dst_shell = 0;
															cur_event_info->shell->init_src_shell = 0;
														
															sstm_msg = get_server_system_msg(30, cur_event_info->name, strlen(cur_event_info->name), -1, &sstm_msg_len);
															if (-1 == write_server_msg(cur_event_info->shell->fd, sstm_msg, sstm_msg_len, 3, 1, 's')){
																clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																			 ctrl_nds, &cur_event_info,  map);
															}	
														}
																				
														cur_event_info->shell->shell = NULL;
														cur_event_info->shell = NULL;				
													}
													else{
														if (-1 == write_server_msg(cur_event_info->shell->fd, cur_event_info->msg_chat_buf, cur_event_info->msg_chat_idx, 3, 0, 's')){
																clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																															ctrl_nds, &cur_event_info,  map);
														}														
													}										 									
												}
												
												else{
													sstm_msg = get_server_system_msg(33, NULL, 0, -1, &sstm_msg_len);
													if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																			 ctrl_nds, &cur_event_info,  map);	
													}										
												}
											}
											else {
												if (1 == cur_event_info->init_dst_shell || 1 == cur_event_info->init_src_shell){
													sstm_msg = get_server_system_msg(33, NULL, 0, -1, &sstm_msg_len);
													if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																			 ctrl_nds, &cur_event_info,  map);	
													}
													if (1 == cur_event_info->init_dst_shell){
														cur_event_info->init_dst_shell = 0;
													}
													else{
														cur_event_info->init_src_shell = 0;
													}
												}
												else if (0 == cur_event_info->init_dst_shell || 0 == cur_event_info->init_src_shell){
													sstm_msg = get_server_system_msg(33, NULL, 0, -1, &sstm_msg_len);
													if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
														clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																			 ctrl_nds, &cur_event_info,  map);	
													}												
												}
											}				
										}
										else{
											sstm_msg = get_server_system_msg(33, NULL, 0, -1, &sstm_msg_len);
											if (-1 == write_server_msg(cur_event_info->fd, sstm_msg, sstm_msg_len, 1, 0, 's')){
												clear_client(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, &gl_idx_limit, &gl_idx, &gl_total_users,
																	 ctrl_nds, &cur_event_info,  map);	
											}		
										}
									}
								}
							}									
							clear_msg_buf(cur_event_info);
						}				
					}			
				}	
			}
		}
	}
	

	fprintf(stderr, "[system]: Server shutdown.\n");
	
	return 0;
}




