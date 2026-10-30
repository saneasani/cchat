

#include "connections.h"



int write_private_msg(event_info *cur_event_info){
	char tmp_buf[MAX_LINE] = "[p][%s]: ";
	static char buf_dst[MAX_LINE];	
	
	size_t buf_ln = 9;

	buf_ln += strlen(cur_event_info->name);	
	
	int n = -1;
	if (-1 == (n = snprintf(buf_dst, MAX_LINE, tmp_buf, cur_event_info->name)))
		err(EXIT_FAILURE, "[system]: create_private_msg, ");	
	
	
	buf_ln += (size_t)n;
	buf_ln += cur_event_info->msg_chat_idx;
	
	if ((buf_ln) >= MAX_LINE){
		write_msg(cur_event_info, "[server] Message is too long.\n", strlen("[server] Message is too long.\n"), 1, 0, 's');
		fprintf(stderr, "[system] create_private_msg, length >= MAX_EVENTS.\n");
		exit(1);
	}
	
	
	strncat(buf_dst, cur_event_info->msg_chat_buf, cur_event_info->msg_chat_idx);	
	
	if ('\0' != buf_dst[buf_ln]){
		fprintf(stderr, "[system]: create_private_msg not null terminating.\n");
		exit(1);
	}
	
	int res1 = -1;
	int res2 = -1;
	
	res1 = write_server_msg(cur_event_info->private, buf_dst, buf_ln, 1, 0, 's');		
	res2 = write_server_msg(cur_event_info, buf_dst, buf_ln, 1, 0, 's');	

	for (int i = 0; i < MAX_LINE; buf_dst[i++] = '\0');
																							
	if (-1 == res1 || -1 == res2)
		return -1;
	
	return 1;
}


void write_global_msg(event_info *cur_event_info, ptr_db *gl_users_db){
	char tmp_buf[MAX_LINE] = "[%s]: ";
	char buf_dst[MAX_LINE] = {0};	
	size_t buf_ln = 6;

	
	int n = -1;
	if (-1 == (n = snprintf(buf_dst, MAX_LINE, tmp_buf, cur_event_info->name)))
		err(EXIT_FAILURE, "write_global_msg, ");	
	
	
	buf_ln += (size_t)n;
	buf_ln += cur_event_info->msg_chat_idx;
	
	if ((buf_ln) >= MAX_LINE){
		write_msg(cur_event_info, "[server] Message is too long.\n", strlen("[server] Message is too long.\n"), 1, 0, 's');
		fprintf(stderr, "[system] write_global_msg, length >= MAX_EVENTS.\n");
		exit(1);
	}

	strncat(buf_dst, cur_event_info->msg_chat_buf, cur_event_info->msg_chat_idx);			
	printf("buf_dst[buf_ln] =>:%d\n", buf_dst[buf_ln]);
	
 	if ('\0' != buf_dst[buf_ln]){
		fprintf(stderr, "[system]: write_global_msg not null terminating.\n");
		exit(1);
	}


	for (size_t i = 0; i < gl_users_db->idx_ptr; i++){
		write_msg(gl_users_db->idx_db[i], buf_dst, buf_ln, 1, 0, 's');
	} 
	
	
	for (int i = 0; i < MAX_LINE; buf_dst[i++] = '\0');			
	
	return;
}


void clear_output_msg(event_info *cur_event_info){
	cur_event_info->ptr = NULL;
	cur_event_info->total_len = 0;
	cur_event_info->cur_len = 0;
	cur_event_info->status_msg = 0;
	cur_event_info->msg_std = (struct message){0};
	cur_event_info->msg_num = (struct message_numbers){0};
	cur_event_info->write = 0;
	cur_event_info->n_left = 0;
	cur_event_info->n_written = 0;
 
}



int write_msg(event_info *cur_event_info, void *ptr_data, size_t in_len, int msg_type, int notif, char msg_struct){
	if (cur_event_info->fd < 0){
		fprintf(stderr, "[system]: write_msg error, fd < 0, %d\n", cur_event_info->fd);
		exit(1);
	}
	
	if (0 == cur_event_info->status_msg){
		clear_output_msg(cur_event_info);

		if ('s' == msg_struct){	
			cur_event_info->msg_std.msg_type = msg_type;
			if (notif) cur_event_info->msg_std.notif = notif;
			else cur_event_info->msg_std.notif= 0;
			
			char *data = ptr_data;		
			memcpy(cur_event_info->msg_std.buf, data, in_len);
			
			if (2 == msg_type){
				cur_event_info->msg_std.len = in_len;
			} else {
				cur_event_info->msg_std.len = strnlen(cur_event_info->msg_std.buf, in_len);
			}
			
			
			size_t tmp_len = 0;
			tmp_len = cur_event_info->msg_std.len;
			cur_event_info->msg_std.len = htons(cur_event_info->msg_std.len);
			
			cur_event_info->total_len = sizeof(cur_event_info->msg_std.msg_type) + sizeof(cur_event_info->msg_std.notif) + sizeof(cur_event_info->msg_std.len) + tmp_len;
			
			cur_event_info->ptr = (char*)&cur_event_info->msg_std;
		}	

		else if ('n' == msg_struct) {	
			cur_event_info->msg_num.msg_type = msg_type;
			if (notif)cur_event_info-> msg_num.notif = notif;
			else cur_event_info->msg_num.notif= 0;
			
			cur_event_info->msg_num.reserved = 0;
			cur_event_info->msg_num.num_data = htonl((*(uint32_t*)ptr_data));
		
			cur_event_info->total_len = sizeof(cur_event_info->msg_num.msg_type) + sizeof(cur_event_info->msg_num.notif) + sizeof(cur_event_info->msg_num.reserved) + sizeof(cur_event_info->msg_num.num_data);

			cur_event_info->ptr = (char*)&cur_event_info->msg_num;	

		}	
		
		cur_event_info->cur_len = 0;
		cur_event_info->n_left = cur_event_info->total_len;
		cur_event_info->n_written = -1;		
		cur_event_info->status_msg = 1;
		cur_event_info->write = 1;
	}


	
	
	 
	while (cur_event_info->n_left > 0) {
		if ((cur_event_info->n_written = write(cur_event_info->fd, cur_event_info->ptr, cur_event_info->n_left)) <= 0) {
 
			
			
			if (cur_event_info->n_written < 0){
				if (EINTR == errno){
					cur_event_info->n_written = 0;	
					continue;		
				}
				else if (EAGAIN == errno || EWOULDBLOCK == errno){
					cur_event_info->n_written = 0;	

					break;		
				}		
				else if (EPIPE == errno){
					clear_output_msg(cur_event_info);
					return -1;
				}
				else if (ECONNRESET == errno){ 
					clear_output_msg(cur_event_info);
					return -1;
				
				} else { 
					err(EXIT_FAILURE, "[system]: write msg error ");	
				}	
			}	
		}
		cur_event_info->n_left  -= cur_event_info->n_written;
		cur_event_info->ptr     += cur_event_info->n_written;
		cur_event_info->cur_len += cur_event_info->n_written;
		
		break;
	}
	

	
	
	
	if (cur_event_info->total_len == cur_event_info->cur_len){	
		clear_output_msg(cur_event_info);
	}
	
	if (0 != cur_event_info->fl_file_name && cur_event_info->left_file_size > 0 && -1 != cur_event_info->event_file_fd){
		write_to_event_fd(cur_event_info->event_file_fd);		
	}
	 
	if (cur_event_info->cur_len > cur_event_info->total_len){
		fprintf(stderr, "[system] cur_len: %ld !=  total_len: %ld, fd: %d\n", cur_event_info->cur_len,
																			  cur_event_info->total_len, 
																			  cur_event_info->fd);
		exit(1);
	
	} 
	
	return 1;
}


int write_server_msg(event_info *cur_event_info, void *ptr_data, size_t in_len, int msg_type, int notif, char msg_struct){
	if (-1 == write_msg(cur_event_info, ptr_data, in_len, msg_type, notif, msg_struct)){
		return -1;
	}
	return 1;
}




char *sock_ntop(const struct sockaddr *sa){
    char portstr[8] = {0};
    static char str[128];		 

	struct sockaddr_in6	*sin6 = (struct sockaddr_in6 *) sa;

	str[0] = '[';
	if (inet_ntop(AF_INET6, &sin6->sin6_addr, str + 1, sizeof(str) - 1) == NULL)
		return(NULL);
	if (ntohs(sin6->sin6_port) != 0) {
		snprintf(portstr, sizeof(portstr), "]:%d", ntohs(sin6->sin6_port));
		strcat(str, portstr);
		return(str);
	}
	return str + 1;
}



int create_tcp_listen_socket(const char *port){
	// --- create dual-stack IPv4/IPv6 socket ---
	int	listen_fd = -1;
	if (-1 == (listen_fd = socket(AF_INET6, SOCK_STREAM, 0))) 
		err(EXIT_FAILURE, "[system]: socket AF_INET6 creation error ");
	
	const int on = 1;
	if (-1 == setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))
		err(EXIT_FAILURE, "[system]: setsockopt on SO_REUSEADDR error ");
		
	const int off = 0;
	if (-1 == setsockopt(listen_fd, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off)))
		err(EXIT_FAILURE, "[system]: setsockopt off IPV6_V6ONLY error ");
			
	struct sockaddr_in6 serv_addr = {0};
	serv_addr.sin6_family = AF_INET6;
	serv_addr.sin6_addr = in6addr_any;
	long num_port = proceed_strtol(port);
	serv_addr.sin6_port = htons(num_port);
	
	if (-1 == bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in6))) 
		err(EXIT_FAILURE, "[system]: bind error ");	
	if (-1 == listen(listen_fd, SOMAXCONN)) 
		err(EXIT_FAILURE, "[system]: listen error ");
	
	return listen_fd;
}


int create_tcp_client_socket(char *addr, char *port){
	if (NULL == addr){
		fprintf(stderr, "[system]: create_tcp_client_socket, addr is null.\n");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in serv_addr = {0};
	memset(&serv_addr,0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	long num_port = proceed_strtol(port);
	serv_addr.sin_port = htons(num_port);
	
	if (inet_pton(AF_INET, addr, &serv_addr.sin_addr) <= 0) {
		fprintf(stderr, "[system]: create_tcp_client_socket, inet_pton error for %s\n", addr);
		exit(1);
    }

	int conn_fd = -1;
    if ((conn_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        err(EXIT_FAILURE, "[system]: socket error ");
	}
		
	if (connect(conn_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
		err(EXIT_FAILURE, "[system]: connect error ");
	}
	
	return conn_fd;
}












int write_msg_client(int conn_fd, int *lock_fd, event_info *cur_event_info, void *ptr_data, size_t in_len, int msg_type, int notif, char msg_struct){
	if (conn_fd < 0){
		fprintf(stderr, "[system]: write_msg_client error, conn_fd < 0, %d\n", conn_fd);
		exit(1);
	}
	
	if (0 == cur_event_info->status_msg){
		clear_output_msg(cur_event_info);

		if ('s' == msg_struct){	
			cur_event_info->msg_std.msg_type = msg_type;
			if (notif) cur_event_info->msg_std.notif = notif;
			else cur_event_info->msg_std.notif= 0;
			
			char *data = ptr_data;		
			memcpy(cur_event_info->msg_std.buf, data, in_len);
			
			if (2 == msg_type){
				cur_event_info->msg_std.len = in_len;
			} else {
				cur_event_info->msg_std.len = strnlen(cur_event_info->msg_std.buf, in_len);
			}
			
			
			size_t tmp_len = 0;
			tmp_len = cur_event_info->msg_std.len;
			cur_event_info->msg_std.len = htons(cur_event_info->msg_std.len);
			
			cur_event_info->total_len = sizeof(cur_event_info->msg_std.msg_type) + sizeof(cur_event_info->msg_std.notif) + sizeof(cur_event_info->msg_std.len) + tmp_len;
			
			cur_event_info->ptr = (char*)&cur_event_info->msg_std;
		}	

		else if ('n' == msg_struct) {	
			cur_event_info->msg_num.msg_type = msg_type;
			if (notif)cur_event_info-> msg_num.notif = notif;
			else cur_event_info->msg_num.notif= 0;
			
			cur_event_info->msg_num.reserved = 0;
			cur_event_info->msg_num.num_data = htonl((*(uint32_t*)ptr_data));
		
			cur_event_info->total_len = sizeof(cur_event_info->msg_num.msg_type) + sizeof(cur_event_info->msg_num.notif) + sizeof(cur_event_info->msg_num.reserved) + sizeof(cur_event_info->msg_num.num_data);

			cur_event_info->ptr = (char*)&cur_event_info->msg_num;	

		}	
		
		cur_event_info->cur_len = 0;
		cur_event_info->n_left = cur_event_info->total_len;
		cur_event_info->n_written = -1;		
		cur_event_info->status_msg = 1;
		cur_event_info->write = 1;
	}
	

	if (-1 == *lock_fd || *lock_fd == cur_event_info->fd){
		while (cur_event_info->n_left > 0) {
			if ((cur_event_info->n_written = write(conn_fd, cur_event_info->ptr, cur_event_info->n_left)) <= 0) {
	 				
				if (cur_event_info->n_written < 0){
					if (EINTR == errno){
						cur_event_info->n_written = 0;	
						continue;		
					}
					else if (EAGAIN == errno || EWOULDBLOCK == errno){
						cur_event_info->n_written = 0;
						
						break;						
										
					}		
					else if (EPIPE == errno){
						clear_output_msg(cur_event_info);
						return -1;
					}
					else if (ECONNRESET == errno){ 
						clear_output_msg(cur_event_info);
						return -1;
					
					} else { 
						err(EXIT_FAILURE, "[system]: write msg error ");	
					}	
				}	
			}
			cur_event_info->n_left  -= cur_event_info->n_written;
			cur_event_info->ptr     += cur_event_info->n_written;
			cur_event_info->cur_len += cur_event_info->n_written;
						
			
			break;
		}
		
		if (-1 == *lock_fd){
			*lock_fd = cur_event_info->fd;
		}
		


		
		if (cur_event_info->total_len == cur_event_info->cur_len){	
			clear_output_msg(cur_event_info);
			*lock_fd = -1;
		}

		
		if (0 != cur_event_info->fl_file_name && cur_event_info->left_file_size > 0 && -1 != cur_event_info->event_file_fd){
			write_to_event_fd(cur_event_info->event_file_fd);		
		}
		 
		if (cur_event_info->cur_len > cur_event_info->total_len){
			fprintf(stderr, "[system] cur_len: %ld !=  total_len: %ld, fd: %d\n", cur_event_info->cur_len,
																				  cur_event_info->total_len, 
																				  cur_event_info->fd);
			exit(1);
		
		} 
	
	} 
	
	return 1;
}




void write_client_msg(int fd, int *lock_fd, event_info *cur_event_info, void *ptr_data, size_t in_len, int msg_type, int notif, char msg_struct){
	if (-1 == write_msg_client(fd, lock_fd, cur_event_info, ptr_data, in_len, msg_type, notif, msg_struct)){
		exit(1);
	}
}





char parse_command(event_info *ev_info, char *buf, size_t ln, char  **ptr_name, int *status, struct winsize *wbuf, size_t *idx, int *fl_line_feed, int *total_usr_lines){
	if (NULL == buf){
		fprintf(stderr, "[system]: parse_command, buf is NULL.\n");
		exit(1);
	}
		
	if (NULL == ptr_name){
		fprintf(stderr, "[system]: parse_command, ptr_name is NULL.\n");
		exit(1);
	}	

	
	
	static char err_msg1[] = "[system]: No such command, can't proceed.\n"; 
	static char err_msg2[] = "[system]: Invalid name, can't proceed.\n";
	static size_t ln_err_msg1 = 42;
	static size_t ln_err_msg2 = 39;
	int fl_found = 0;
	
	if (ln < 4 && 'e' != buf[0]){ 
		if (NULL == ev_info){
			write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
		} else {
			write_msg(ev_info, err_msg1, ln_err_msg1, 1, 0, 's');
		}
			
		return 'n';
	}
	
	
	if ('e' == buf[0] || 'f' == buf[1] || 's' == buf[1] || 
		'b' == buf[1] || 'e' == buf[1] || 'o' == buf[1] || 
		'g' == buf[1] || 'p' == buf[1] || 'c' == buf[1]){}
	else{ 
		if (NULL == ev_info){
			write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
		} else {
			write_msg(ev_info, err_msg1, ln_err_msg1, 1, 0, 's');
		}
			
		return 'n';
	}
	
	static char e[] = "exit", fl[] = "/file/", ex[] = "/exit", cl[] = "/clear", sh[] = "/shell/", br[] = "/browse", on[] = "/online", gl[] = "/global", pr[] = "/private/"; 
	static char name[10];  
	int count_ok = 0, fl_name = 0, fl_clear = 0, result = 0;
	size_t j = 0;
	
		
	
	if (NULL == *ptr_name)
		*ptr_name = name;

	for (size_t i = 1; i < ln-1; i++){ // cut '\n'
	
		if ('f' == buf[1]){ // /file
			result =  filter_command(buf[1], buf, fl, i, &j, name, &fl_name, &count_ok, &status);	
			
			if (-1 == result || 0 == result){
				fl_clear = 1;
				break;	
			}
		}
			
			
		else if ('e' == buf[0]){ // exit	
			if (i > 3 || buf[i] != e[i] ){
				break;
			}	
		}	

		else if ('e' == buf[1]){ // /exit	
			if (i > 4 || buf[i] != ex[i] ){
				if (NULL == ev_info){
					write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
				} else {
					write_msg(ev_info, err_msg1, ln_err_msg1, 1, 0, 's');
				}
				
				return 'n';
			}
		}	
		
		else if ('c' == buf[1]){ // /clear
			if (i > 5 || buf[i] != cl[i] ){
				if (NULL == ev_info) {
					write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
				} else {
					write_msg(ev_info, err_msg1, ln_err_msg1, 1, 0, 's');
				}
				
				return 'n';
			}
		}		
		
		
			
		else if ('s' == buf[1]){
			result =  filter_command(buf[1], buf, sh, i, &j, name, &fl_name, &count_ok, &status);
			
			if (-1 == result || 0 == result){
				fl_clear = 1; 
				break;
			}
		}	
		
		else if ('b' == buf[1]){  // /browse
			if (i > 6 || buf[i] != br[i]){
				if (NULL == ev_info) {
					write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
				} else {
					write_msg(ev_info, err_msg1, ln_err_msg1, 1, 0, 's');
				}
				
				return 'n';
			}
		}	
	
		else if ('o' == buf[1]){ 
			if (i > 6 || buf[i] != on[i]){
				if (NULL == ev_info) {
					write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);;
				} else {
					write_msg(ev_info, err_msg1, ln_err_msg1, 1, 0, 's');
				}
				
				return 'n';
			}
		}
		
		else if ('g' == buf[1]){ 
			if (i > 6 || buf[i] != gl[i]){
				if (NULL == ev_info) {
					write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
				} else {
					write_msg(ev_info, err_msg1, ln_err_msg1, 1, 0, 's');
				}
				
				return 'n';
			}
		}
		
		else if ('p' == buf[1]){
			result =  filter_command(buf[1], buf, pr, i, &j, name, &fl_name, &count_ok, &status);	
			
			if (-1 == result || 0 == result){
				fl_clear = 1; 
				break;
			}
		}
	
		
		
		if (('e' == buf[0]) && (i == ln-1))
			fl_found++;
		
		else if (i+1 == ln-1)
			fl_found++;	
		
	}
	
	if (!fl_found){
		if (buf[0] == 'e')
			return '\0';
		if (NULL == ev_info) {
			write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
		} else {
			write_msg(ev_info, err_msg1, ln_err_msg1, 1, 0, 's');
		}
		
		return 'n';
	}
	
	
	if ('e' == buf[0])
		return buf[0];
	
	if ('b' == buf[1] || 'e' == buf[1] || 'o' == buf[1] || 'g' == buf[1] || 'c' == buf[1])
		return buf[1];
	

	name[j] = '\0';
	
	
	if (1 == fl_clear){
		if (0 == result){
			if (NULL == ev_info) {
				write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
			} else {
				write_msg(ev_info, err_msg1, ln_err_msg1, 1, 0, 's');
			}
		}
		else if (-1 == result || 3 > strnlen(name, sizeof(name))){
			if (NULL == ev_info){
				write_common_console_msg(err_msg2, wbuf, idx, ln_err_msg2, fl_line_feed, total_usr_lines, buf);
			} else {
				write_msg(ev_info, err_msg2, ln_err_msg2, 1, 0, 's');
			}
		}
			
		memset(name, 0 , j);
		*status = -1;
		
		return 'n';	
	}

	
	return buf[1];
}



 

int read_incoming_data(ssize_t *n_read, ssize_t *in_idx, char buf_in[], event_info* data_usr){
	if (NULL == n_read){
		fprintf(stderr, "[system]: read_incoming_data, n_read is NULL.\n");
		exit(1);
	}
	if (NULL == in_idx){
		fprintf(stderr, "[system]: read_incoming_data, in_idx is NULL.\n");
		exit(1);
	}
	if (NULL == buf_in){
		fprintf(stderr, "[system]: read_incoming_data, buf is NULL.\n");
		exit(1);
	}	
	
 
	if (0 == data_usr->start_msg){	
		if (*in_idx < *n_read && data_usr->msg_chat_idx < MAX_LINE && *in_idx < CHUNK_SIZE){
			data_usr->start_msg = 1; 
			data_usr->type_msg = buf_in[(*in_idx)++];
		
		} else {
			return 0;
		}
	
	}
	if (1 == *n_read && 1 == data_usr->start_msg){
		data_usr->start_msg = -1;
		return 0;
	}
	
	
	if (!data_usr->fl_notif_rcv){
		if (*in_idx < *n_read && data_usr->msg_chat_idx < MAX_LINE && *in_idx < CHUNK_SIZE){
			data_usr->notif_rcv = buf_in[(*in_idx)++];
			data_usr->fl_notif_rcv = 1;
		
		} else {
			return 0;
		}
	}



	if (1 == data_usr->type_msg 												// standard chat
	|| (0 == data_usr->fl_file_name	  && 2 == data_usr->type_msg)				// file name
	|| (0 == data_usr->fl_file_chunks && 2 == data_usr->type_msg 
									  && 1 == data_usr->fl_file_size)){ 		// chunks of file						
		if (0 == data_usr->flag_len_msg){	
			if (*in_idx < *n_read && data_usr->msg_chat_idx < MAX_LINE && *in_idx < CHUNK_SIZE){
				data_usr->len_chat_msg_arr[0] = buf_in[(*in_idx)++];
				data_usr->flag_len_msg++;
				
				if (1 == *n_read)
					return 0;
			} else {
				return 0;
			}
			
		}		
		if (1 == data_usr->flag_len_msg){
			if (*in_idx < *n_read && data_usr->msg_chat_idx < MAX_LINE && *in_idx < CHUNK_SIZE){
				data_usr->len_chat_msg_arr[1] = buf_in[(*in_idx)++];
				data_usr->len_chat_msg = ntohs(*(uint16_t*)data_usr->len_chat_msg_arr);
				data_usr->flag_len_msg++;
				
				if (data_usr->len_chat_msg > 1026){
					static char ans[] = "[system]: read_incoming_data, Non supported len of message, maximum is 1026.\n";
					fprintf(stderr, "%s", ans);

					return -1;				
				}
		
				if (1 == *n_read)
					return 0;
			
			} else {
				return 0;
			}
		}
		
		for (;*in_idx < *n_read && data_usr->msg_chat_idx < MAX_LINE && data_usr->msg_chat_idx < data_usr->len_chat_msg && *in_idx < CHUNK_SIZE;)
			data_usr->msg_chat_buf[data_usr->msg_chat_idx++] = buf_in[(*in_idx)++];
		
		
 
		if (data_usr->msg_chat_idx == data_usr->len_chat_msg){
			return 1;
		}
		else{
			return 0;
		}

		
	
	}
	
	
	else if (2 == data_usr->type_msg){	// file transfer
			if (0 == data_usr->flag_reserved_msg){		
				if (data_usr->msg_chat_idx < MAX_LINE && *in_idx < CHUNK_SIZE){
					data_usr->len_reserved_arr[0] = buf_in[(*in_idx)++];
					data_usr->flag_reserved_msg++;
				
					if (1 == *n_read)
						return 0;
				
				} else {
					return 0;
				}
			}	
			
			if (1 == data_usr->flag_reserved_msg){	
				if (*in_idx < *n_read && data_usr->msg_chat_idx < MAX_LINE && *in_idx < CHUNK_SIZE){
					data_usr->len_reserved_arr[1] = buf_in[(*in_idx)++];
					data_usr->reserved = ntohs(*(uint16_t*)data_usr->len_reserved_arr);
					data_usr->flag_reserved_msg++;
					
					if (1 == *n_read)
						return 0;
				
				} else {
					return 0;
				}
			}
			
			for (;*in_idx < *n_read && data_usr->msg_file_idx < 4 && *in_idx < CHUNK_SIZE;){
				data_usr->num_data_arr[data_usr->msg_file_idx++] = buf_in[(*in_idx)++];
			}
			
			
			if (4 == data_usr->msg_file_idx){
				data_usr->num_data = ntohl(*(uint32_t*)data_usr->num_data_arr);
				
				return 1;
			}
				
	}
	

	
	else if (3 == data_usr->type_msg) { // shell managing
		if (0 == data_usr->flag_len_msg){		
			if (*in_idx < *n_read && data_usr->msg_chat_idx < MAX_LINE && *in_idx < CHUNK_SIZE){
				data_usr->len_chat_msg_arr[0] = buf_in[(*in_idx)++];
				data_usr->flag_len_msg++;
				
				if (1 == *n_read)
					return 0;
			
			} else {
				return 0;
			}
			
		}		
		if (1 == data_usr->flag_len_msg){
			if (*in_idx < *n_read && data_usr->msg_chat_idx < MAX_LINE && *in_idx < CHUNK_SIZE){
				data_usr->len_chat_msg_arr[1] = buf_in[(*in_idx)++];
				data_usr->len_chat_msg = ntohs(*(uint16_t*)data_usr->len_chat_msg_arr);
				data_usr->flag_len_msg++;
				
				if (data_usr->len_chat_msg > 1026){
					static char ans3[] = "[system]: read_incoming_data, Non supported len of message, maximum is 1026.\n";
					fprintf(stderr, "%s", ans3);
					return -1;				
				}

				
				if (1 == *n_read)
					return 0;
			
			} else {
				return 0;
			}
			
		}
		
		for (;*in_idx < *n_read && data_usr->msg_chat_idx < MAX_LINE && data_usr->msg_chat_idx < data_usr->len_chat_msg && *in_idx < CHUNK_SIZE;)
			data_usr->msg_chat_buf[data_usr->msg_chat_idx++] = buf_in[(*in_idx)++];
				
		
		if (data_usr->msg_chat_idx  == data_usr->len_chat_msg){
			return 1;
		}
		else{
			return 0;
		}
	}
	
	return -1;
}
	 
