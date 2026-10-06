
#include "common.h"



 
 void write_n(int fd, char *msg, size_t ln){
	if (fd < 0){
		fprintf(stderr, "[system]: write_n, fd < 0.\n");
		exit(1);
	}
	if (ln < 0){
		fprintf(stderr, "[system]: write_n, ln < 0.\n");
		exit(1);
	}
	if (NULL == msg){
		fprintf(stderr, "[system]: write_n, msg is NULL.\n");
		exit(1);
	}
	
	int ret = -1;

	while (1){
		if (-1 == (ret = write(fd, msg, ln))){
			if (EINTR == errno || EAGAIN == errno || EWOULDBLOCK == errno)
				continue;
			err(EXIT_FAILURE, "[system]: write_n error ");
		}
		else if (ln != ret){
			fprintf(stderr, "[system]: write_n, ln != rt.\n");
			exit(1);
		} else {
			return;
		}
	}
}


void write_to_event_fd(uint64_t *u, int fd){
	if (NULL == u){
		fprintf(stderr, "[system]: write_to_event_fd, uint64_t is null.\n");
		exit(1);
	}
	if (fd < 0){
		fprintf(stderr, "[system]: write_to_event_fd, fd < 0.\n");
		exit(1);
	}
	*u = 1;
	ssize_t s = -1;
	s = write(fd, &u, UINT64_T_SZ);
	if (s != UINT64_T_SZ)
		err(EXIT_FAILURE, "[system]: write_to_event_fd error ");			
}


void fun_on_exit(int exit_status, void *arg){
	struct signal_struct *sig_strct = arg;
	
	if (1 == *sig_strct->sig_occured){
		*sig_strct->sig_occured = 0;
		
		if (NULL != sig_strct->prev_settings){	
			if (-1 == tcsetattr(STDIN_FILENO, TCSAFLUSH, sig_strct->prev_settings))
				err(EXIT_FAILURE, "[system]: tcgetattr, ");
			fprintf(stderr, "[system]: Shutting down client. Settinng back tty settings.\n");
		
		} else {
			fprintf(stderr, "[system]: Error on restoring a tty settings.\n");
		}
	}
}

 

int filter_command(char command, char *buf, char *comparison_buf, size_t i, size_t *j, char *name, int *flag_name, int *count_ok, int **status){
	if (NULL == comparison_buf){
		fprintf(stderr, "[system]: filter_command, comparison_buf is NULL.\n");
		exit(1);
	}
	if (NULL == comparison_buf){
		fprintf(stderr, "[system]: filter_command, i < 0.\n");
		exit(1);
	}	

	
	if ('f' == command && i <= 5 && buf[i] != comparison_buf[i]) { return 0; } // /file/
	if ('s' == command && i <= 6 && buf[i] != comparison_buf[i]) { return 0; } // /shell/
	if ('p' == command && i <= 8 && buf[i] != comparison_buf[i]) { return 0; } // /private/
		
	if (('f' == command && i > 5) || // /file/username or /file/ok/username
		('s' == command && i > 6) || // /shell/username or /shell/ok/username
		('p' == command && i > 8)    // /private/username or /private/ok/username
		){ 
			if (('f' == command && 6 == i && buf[i] != 'o') ||
				('s' == command && 7 == i && buf[i] != 'o') ||
				('p' == command && 9 == i && buf[i] != 'o') 
				) *flag_name = 1;
		
			else if (('f' == command && 6 == i && buf[i] == 'o') ||
				('s' == command && 7 == i && buf[i] == 'o') ||
				('p' == command && 9 == i && buf[i] == 'o') 
				) (*count_ok)++;

			else if (('f' == command && 7 == i && buf[i] == 'k' && 1 == *count_ok) ||
				('s' == command && 8 == i && buf[i] == 'k' && 1 == *count_ok) ||
				('p' == command && 10 == i && buf[i]== 'k' && 1 == *count_ok) 
				) (*count_ok)++;	
			
			else if (('f' == command && 7 == i && buf[i] == 'k' && 0 == *count_ok) ||
				('s' == command && 8 == i && buf[i] == 'k' && 0 == *count_ok) ||
				('p' == command && 10 == i && buf[i]== 'k' && 0 == *count_ok) 
				) *flag_name = 1; 	

			else if (('f' == command && 7 == i && buf[i] != 'k') ||
				('s' == command && 8 == i && buf[i] != 'k') ||
				('p' == command && 10 == i && buf[i]!= 'k') 
				) *flag_name = 1;
			
			// /command/username 
			if (1 == (*flag_name)){ 
				if (-1 == (**status))
					**status = 1;
			
			
				if (('f' == command && i > 14) ||
				    ('s' == command && i > 15) ||				
					('p' == command && i > 17)
				) return -1; //  // limit len of /command/username

				if (!isalpha(buf[i]) && !isdigit(buf[i]))
					return -1;
				
				name[(*j)++] = buf[i];
				return 1;
			}
			
			if (-1 == (**status))
				**status = 0;
			
			// /command/ok/username 
			if (('f' == command && i  >= 8) ||
				('s' == command && i  >= 9) ||
				('p' == command && i >= 11)
				){			
				if (('f' == command && 8  == i && '/' == buf[i]) ||
					('s' == command && 9  == i && '/' == buf[i]) ||
					('p' == command && 11 == i && '/' == buf[i])
					) return 1; 
					
				if (('f' == command && 8  == i && '/' != buf[i]) ||
					('s' == command && 9  == i && '/' != buf[i]) ||
					('p' == command && 11 == i && '/' != buf[i])
					) return 0;
						
				if (('f' == command && i > 17) ||
					('s' == command && i > 18) ||
					('p' == command && i > 20)
					) return -1; // limit len of /command/ok/username
						
				if (!isalpha(buf[i]) && !isdigit(buf[i]))
					return -1;
						
						
				name[(*j)++] = buf[i];
				return 1;
			}
	}
	
	return 1;
}
	 
	 


char parse_command(int fd, char *buf, size_t ln, char  **ptr_name, int *status, struct winsize *wbuf, size_t *idx, int *fl_line_feed, int *total_usr_lines){
	if (NULL == buf){
		fprintf(stderr, "[system]: parse_command, buf is NULL.\n");
		exit(1);
	}
	if (ln < 0){
		fprintf(stderr, "[system]: parse_command, ln < 0.\n");
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
		if (-1 == fd){
			write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
		} else {
			write_msg(fd, err_msg1, ln_err_msg1, 1, 0, 's');
		}
			
		return 'n';
	}
	
	
	if ('e' == buf[0] || 'f' == buf[1] || 's' == buf[1] || 
		'b' == buf[1] || 'e' == buf[1] || 'o' == buf[1] || 
		'g' == buf[1] || 'p' == buf[1] || 'c' == buf[1]){}
	else{ 
		if (-1 == fd){
			write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
		} else {
			write_msg(fd, err_msg1, ln_err_msg1, 1, 0, 's');
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
				if (-1 == fd){
					write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
				} else {
					write_msg(fd, err_msg1, ln_err_msg1, 1, 0, 's');
				}
				
				return 'n';
			}
		}	
		
		else if ('c' == buf[1]){ // /clear
			if (i > 5 || buf[i] != cl[i] ){
				if (-1 == fd) {
					write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
				} else {
					write_msg(fd, err_msg1, ln_err_msg1, 1, 0, 's');
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
				if (-1 == fd) {
					write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
				} else {
					write_msg(fd, err_msg1, ln_err_msg1, 1, 0, 's');
				}
				
				return 'n';
			}
		}	
	
		else if ('o' == buf[1]){ 
			if (i > 6 || buf[i] != on[i]){
				if (-1 == fd) {
					write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);;
				} else {
					write_msg(fd, err_msg1, ln_err_msg1, 1, 0, 's');
				}
				
				return 'n';
			}
		}
		
		else if ('g' == buf[1]){ 
			if (i > 6 || buf[i] != gl[i]){
				if (-1 == fd) {
					write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
				} else {
					write_msg(fd, err_msg1, ln_err_msg1, 1, 0, 's');
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
		if (-1 == fd) {
			write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
		} else {
			write_msg(fd, err_msg1, ln_err_msg1, 1, 0, 's');
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
			if (-1 == fd) {
				write_common_console_msg(err_msg1, wbuf, idx, ln_err_msg1, fl_line_feed, total_usr_lines, buf);
			} else {
				write_msg(fd, err_msg1, ln_err_msg1, 1, 0, 's');
			}
		}
		else if (-1 == result || 3 > strnlen(name, sizeof(name))){
			if (-1 == fd){
				write_common_console_msg(err_msg2, wbuf, idx, ln_err_msg2, fl_line_feed, total_usr_lines, buf);
			} else {
				write_msg(fd, err_msg2, ln_err_msg2, 1, 0, 's');
			}
		}
			
		memset(name, 0 , j);
		*status = -1;
		
		return 'n';	
	}

	
	return buf[1];
}







 

int read_incoming_data(ssize_t *n_read, size_t *in_idx, char buf_in[], event_info* data_usr){
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
		data_usr->start_msg = 1;
		data_usr->type_msg = buf_in[(*in_idx)++];
	
	}
	if (1 == *n_read && 1 == data_usr->start_msg){
		data_usr->start_msg = -1;
		return 0;
	}
	
	
	if (!data_usr->fl_notif_rcv){
		data_usr->notif_rcv = buf_in[(*in_idx)++];
		data_usr->fl_notif_rcv = 1;
	}



	if (1 == data_usr->type_msg 												// standard chat
	|| (0 == data_usr->fl_file_name	  && 2 == data_usr->type_msg)				// file name
	|| (0 == data_usr->fl_file_chunks && 2 == data_usr->type_msg 
									  && 1 == data_usr->fl_file_size)){ 		// chunks of file						
		if (0 == data_usr->flag_len_msg){	
			data_usr->len_chat_msg_arr[0] = buf_in[(*in_idx)++];
			data_usr->flag_len_msg++;
			if (1 == *n_read)
				return 0;
		}		
		if (1 == data_usr->flag_len_msg){
			data_usr->len_chat_msg_arr[1] = buf_in[(*in_idx)++];
			data_usr->len_chat_msg = ntohs(*(uint16_t*)data_usr->len_chat_msg_arr);
			data_usr->flag_len_msg++;
			
			if (data_usr->len_chat_msg > 1026){
				static char ans[] = "[system]: read_incoming_data, Non supported len of message, maximum is 1026.\n";
				fprintf(stderr, ans);

				return -1;				
			}

			
			if (1 == *n_read)
				return 0;
		}
		
		for (;data_usr->msg_chat_idx < MAX_LINE && data_usr->msg_chat_idx < data_usr->len_chat_msg && *in_idx < CHUNK_SIZE;)
			data_usr->msg_chat_buf[data_usr->msg_chat_idx++] = buf_in[(*in_idx)++];
		
		
 
		if (data_usr->msg_chat_idx  == data_usr->len_chat_msg){
			return 1;
		}
		else{
			return 0;
		}

		
	
	}
	
	
	else if (2 == data_usr->type_msg){	// file transfer
			if (0 == data_usr->flag_reserved_msg){					
				data_usr->len_reserved_arr[0] = buf_in[(*in_idx)++];
				data_usr->flag_reserved_msg++;
				
				if (1 == *n_read)
					return 0;
			}		
			if (1 == data_usr->flag_reserved_msg){	
				data_usr->len_reserved_arr[1] = buf_in[(*in_idx)++];
				data_usr->reserved = ntohs(*(uint16_t*)data_usr->len_reserved_arr);
				data_usr->flag_reserved_msg++;
				
				if (1 == *n_read)
					return 0;
			}
			
			for (;data_usr->msg_file_idx < 4 && *in_idx < CHUNK_SIZE;){
				data_usr->num_data_arr[data_usr->msg_file_idx++] = buf_in[(*in_idx)++];
			}
			
			
			if (4 == data_usr->msg_file_idx){
				data_usr->num_data = ntohs(*(uint32_t*)data_usr->num_data_arr);
				
				return 1;
			}
				
	}
	

	
	else if (3 == data_usr->type_msg) { // shell managing
		if (0 == data_usr->flag_len_msg){		
			data_usr->len_chat_msg_arr[0] = buf_in[(*in_idx)++];
			data_usr->flag_len_msg++;
			
			if (1 == *n_read)
				return 0;
		}		
		if (1 == data_usr->flag_len_msg){
			data_usr->len_chat_msg_arr[1] = buf_in[(*in_idx)++];
			data_usr->len_chat_msg = ntohs(*(uint16_t*)data_usr->len_chat_msg_arr);
			data_usr->flag_len_msg++;
			
			if (data_usr->len_chat_msg > 1026){
				static char ans3[] = "[system]: read_incoming_data, Non supported len of message, maximum is 1026.\n";
				fprintf(stderr, ans3);
				return -1;				
			}

			
			if (1 == *n_read)
				return 0;
		}
		
		for (;data_usr->msg_chat_idx < MAX_LINE && data_usr->msg_chat_idx < data_usr->len_chat_msg && *in_idx < CHUNK_SIZE;)
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
	 
	 
	 
int push_neg_num_to_end_n_reduce_idx(ssize_t *db_idx, ssize_t *db_fd, size_t nums_size) {
	if (NULL == db_idx){ 
		fprintf(stderr, "[system]: move_zeroes: recevied negative db_idx.\n");
		exit(1);
	}
	if (NULL == db_fd){ 
		fprintf(stderr, "[system]: move_zeroes: recevied negative db_fd.\n");
		exit(1);
	}		
	if (0 == nums_size){ 
		fprintf(stderr, "[system]: move_zeroes: recevied nums_size.\n");
		exit(1);
	}
	

    ssize_t *cur = db_idx, *main = db_idx, *e = db_idx+nums_size;
    size_t t = -1;
	


    for(;cur<=e;cur++){
        if(-1 != *main) {
			++main;
        }			
        else if(-1 == *main){  
            if(-1 != *cur){
                db_fd[*cur]--;
                t = *main;
                *main++ = *cur;
                *cur = t;
            }
        }      
    }
	return 1;
}





void put_el_db_num(ssize_t **db_num, ssize_t **db_idx, size_t *num_limit, size_t *idx_limit, size_t *idx, int num){
	if (NULL ==  *db_num){ 
		fprintf(stderr, "[system]: put_el_db_num, recevied null in db_num.\n"); 
		exit(1);
	}
	if (NULL == *db_idx){ 
		fprintf(stderr, "[system]: put_el_db_num, recevied null in db_idx.\n");
		exit(1); 
	}		
	if (*num_limit < 0){ 
		fprintf(stderr, "[system]: put_el_db_num, recevied negative num_limit.\n"); 
		exit(1);
	}	
	if (*idx_limit < 0){ 
		fprintf(stderr, "[system]: put_el_db_num, recevied negative idx_limit.\n"); 
		exit(1);
	}			
	if (num < 0){ 
		fprintf(stderr, "[system]: put_el_db_num, recevied negative num.\n");
		exit(1);
	}		
	if (*idx < 0){ 
		fprintf(stderr, "[system]: put_el_db_num, recevied negative idx.\n");
		exit(1);
	}

 
	if (num < *num_limit && -1 != (*db_num)[num]){ 
		fprintf(stderr, "[system]: num is already in db_num; put_el_db_num \n");
		exit(1);
	}


	while (num >= (*num_limit)){
	    size_t tmp_limit = *num_limit;
		*num_limit *= 2;

		ssize_t *tmp = *db_num;
		if (NULL == (tmp = erealloc(*db_num, *num_limit * sizeof(ssize_t)))){
			exit(1);
		} else {
			  for (int i = tmp_limit; i < *num_limit; tmp[i++] = -1);
			  *db_num = tmp;
		  }
	}
	
	if (*idx >= *idx_limit){
	    size_t tmp_limit = *idx_limit;
		*idx_limit *= *idx_limit; 		
		
		ssize_t *tmp = *db_idx;
		if (NULL == (tmp = erealloc(*db_idx, *idx_limit* sizeof(ssize_t)))){
		    exit(1);
		 }else{
		      for (int i = tmp_limit; i < *idx_limit; tmp[i++] = -1);
		      *db_idx = tmp;
		  }
	}	
	  
	(*db_idx)[*idx] = num;
	(*db_num)[num] = (*idx);
	(*idx)++;
}





int del_el_db_num(ssize_t *db_num, ssize_t *db_idx, size_t num_limit, size_t *idx_limit, size_t *idx, int num){
	if (NULL == db_num){ 
		fprintf(stderr, "[system]: del_el_db_num, db_num is NULL.\n"); 
		return -1;
	}	
	if (NULL == db_idx){ 
		fprintf(stderr, "[system]: del_el_db_num, db_idx is NULL.\n");
		return -1;
	}		
	if (num_limit < 0){ 
		fprintf(stderr, "[system]: del_el_db_num, num_limit < 0.\n");
		return -1;
	}	
	if (num < 0){ 
		fprintf(stderr, "[system]: del_el_db_num, file_num < 0.\n");
		return -1;
	}
	if (num >= num_limit){
		fprintf(stderr, "[system]: del_el_db_num, num >= num_limit.\n");
		return -1; 
	}
	
			
	if (-1 == db_num[num]){ 
		fprintf(stderr, "[system]: num is not in db_num, del_el_db_num.\n");
		return -1;
	}
	
	
	size_t i = db_num[num];	
	db_idx[i] = -1;
	
	if (((*idx)-1 != i)){
		if (-1 == push_neg_num_to_end_n_reduce_idx(db_idx, db_num, (*idx_limit - 1)))
			return -1;
	}
	
	db_num[num] = -1;
	(*idx)--;
	
	
	return 1;
}


 
void clear_msg_buf(event_info *cur_event_info){
	if (NULL == cur_event_info){
		fprintf(stderr, "[system]: clear_msg_buf, entry is NULL.\n");
		exit(1);
	}

	memset(cur_event_info->msg_chat_buf, 0, cur_event_info->msg_chat_idx);
	cur_event_info->msg_chat_idx = 0;
	
	cur_event_info->start_msg = 0;
	cur_event_info->flag_len_msg = 0;
	cur_event_info->type_msg = 0; 
	cur_event_info->len_chat_msg = 0;					
	cur_event_info->notif_rcv = 0;	
	cur_event_info->fl_notif_rcv = 0;		

}


void clear_file_send(event_info *cur_event_info, int fl, int cl_or_sr){
	if (NULL == cur_event_info){
		fprintf(stderr, "[system]: clear_file_send, entry is null.\n");
		exit(1);
	}

	if ('d' == fl){
		if (-1 == close(cur_event_info->file_read_fd))
			err(EXIT_FAILURE, "[system]: error file close, ");
		cur_event_info->file_read_fd = -1;	
	}
	else if ('s' == fl){
		if (-1 == close(cur_event_info->file_wr_fd))
			err(EXIT_FAILURE, "[system]: error file close, ");
		cur_event_info->file_wr_fd = -1;
	}
	else{
		fprintf(stderr, "[system]: clear_file_send, unsupported flag: %c.\n", fl);
		exit(1);		
	}
	
 
	cur_event_info->fl_file_dst_init = 0;
	cur_event_info->fl_file_name = 0;
	cur_event_info->fl_file_size = 0;
	
	cur_event_info->name_file_size = 0;
	cur_event_info->file_size = 0;
	cur_event_info->left_file_size = 0;
	cur_event_info->offset_file = 0;
	cur_event_info->rem_file = 0;
	
	if ('s' == cl_or_sr){
		cur_event_info->file->file = NULL;
		cur_event_info->file = NULL;
		
	}
	
	

	for (int i = 0; i < MAX_LINE; cur_event_info->file_name[i++] = 0);		
	for (int i = 0; i < CHUNK_SIZE; cur_event_info->file_chunk[i++] = 0);	
}																

void clear_client(size_t *db_gl_chat_fd, size_t *db_gl_chat_idx, size_t gl_fd_limit, size_t *gl_idx_limit, size_t *idx, size_t *gl_total_usrs, 
				  control_nodes *cn, event_info **node, hash_map *map){
				  
	del_el_db_num(db_gl_chat_fd, db_gl_chat_idx, gl_fd_limit, gl_idx_limit, idx, (*node)->fd);
	
	
	printf("[(*node)->entry_idx] =>:%d\n", (*node)->entry_idx);
	printf("[(*node)->fd)] =>:%d\n", (*node)->fd);
	
	clear_entry_by_idx_fd(map, (*node)->entry_idx, (*node)->fd);

	if (-1 == close((*node)->fd))
		err(EXIT_FAILURE,  "[server]: clear_client: close fd error.\n");
	pop_node(cn, *node);
	*node = NULL;
	
	(*gl_total_usrs)--;
}




int pty_master_open(char *slave_name, size_t sn_len){
    int master_fd, saved_errno;
    char *p;

    master_fd = posix_openpt(O_RDWR | O_NOCTTY);      
    if (1 == master_fd)
        return -1;

    if (1 == grantpt(master_fd)) {            
        saved_errno = errno;
        close(master_fd);                       
        errno = saved_errno;
        return -1;
    }

    if (1 == unlockpt(master_fd)) {              
        saved_errno = errno;
        close(master_fd);                       
        errno = saved_errno;
        return -1;
    }

    p = ptsname(master_fd);                     
    if (NULL == p) {
        saved_errno = errno;
        close(master_fd);                       
        errno = saved_errno;
        return -1;
    }

    if (strlen(p) < sn_len) {
        strncpy(slave_name, p, sn_len);
    } else {                     
        close(master_fd);
        errno = EOVERFLOW;
        return -1;
    }

    return master_fd;
}



pid_t pty_fork(int *master_fd, char *slave_name, size_t sn_len,
        const struct termios *slave_termios, const struct winsize *slave_ws){
			
    int mfd, slave_fd, saved_errno;
    pid_t child_pid;
    char sl_name[MAX_SNAME];

    mfd = pty_master_open(sl_name, MAX_SNAME);
    if (-1 == mfd)
        return -1;
	


    if (NULL != slave_name) {          
        if (strlen(sl_name) < sn_len) {
            strncpy(slave_name, sl_name, sn_len);

        } else {                      
            close(mfd);
            errno = EOVERFLOW;
            return -1;
        }
    }

    child_pid = fork();

    if (-1 == child_pid) {             
        saved_errno = errno;           
        close(mfd);                    
        errno = saved_errno;
        return -1;
    }

    if (0 != child_pid) {               
        *master_fd = mfd;            
        return child_pid;             
    }

 

    if (-1 == setsid())                 
        err(EXIT_FAILURE, "[system]: pty_fork setsid");

    close(mfd);                      

    slave_fd = open(sl_name, O_RDWR);   
    if (-1 == slave_fd)
        err(EXIT_FAILURE, "[system]: pty_fork open-slave");

	#ifdef TIOCSCTTY                       
    if (-1 == ioctl(slave_fd, TIOCSCTTY, 0))
        err(EXIT_FAILURE, "[system]: pty_fork ioctl-TIOCSCTTY");
	#endif

    if (NULL != slave_termios)          
        if (-1 == tcsetattr(slave_fd, TCSANOW, slave_termios))
            err(EXIT_FAILURE, "[system]: pty_fork tcsetattr");

    if (NULL != slave_ws)                
        if (-1 == ioctl(slave_fd, TIOCSWINSZ, slave_ws))
            err(EXIT_FAILURE, "[system]: pty_fork ioctl-TIOCSWINSZ");

	

    if (dup2(slave_fd, STDIN_FILENO) != STDIN_FILENO)
        err(EXIT_FAILURE, "[system]: pty_fork, dup2-STDIN_FILENO ");
    if (dup2(slave_fd, STDOUT_FILENO) != STDOUT_FILENO)
        err(EXIT_FAILURE, "[system]: pty_fork, dup2-STDOUT_FILENO ");
    if (dup2(slave_fd, STDERR_FILENO) != STDERR_FILENO)
        err(EXIT_FAILURE, "[system]: pty_fork, dup2-STDERR_FILENO ");
	
	

    if (slave_fd > STDERR_FILENO)        
        close(slave_fd);                 

    return 0;                           
}




void set_non_block_buf_in_out_err(){
	if (0 != setvbuf(stdin, NULL, _IONBF, 0))
		err(EXIT_FAILURE, "[system]: se_non_bloc_buf stdin "); 

	if (0 != setvbuf(stdout, NULL, _IONBF, 0))
		err(EXIT_FAILURE, "[system]: se_non_bloc_buf stdout ");

	if (0 != setvbuf(stderr, NULL, _IONBF, 0))
		err(EXIT_FAILURE, "[system]: se_non_bloc_buf stdderr ");
}



 
void set_non_block_fd(int fd){
	int flags = -1;
	if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
		err(EXIT_FAILURE, "[system]: fcntl F_GETFL error ");
	if (-1 == fcntl(fd, F_SETFL, flags | O_NONBLOCK))
		err(EXIT_FAILURE, "[system]: fcntl F_SETFL error ");
}



void wait_children(){
	int status = -1;
	pid_t kid_pid = -1;
	
	while (0 < (kid_pid = waitpid(-1, &status, WNOHANG))){}
}


void catcher(int sig_num){
	if (SIGQUIT == sig_num){}
	else if (SIGINT == sig_num){}
	
	else if(SIGTERM == sig_num){
		exit(1);
	}
	else if (SIGCHLD == sig_num){
		wait_children();
	}
}


void set_signals(){
	struct sigaction sig_cntrl = {0};
	sig_cntrl.sa_flags = 0;
	sig_cntrl.sa_handler = catcher;

	if (0 != sigaction(SIGINT, &sig_cntrl, NULL))
			err(EXIT_FAILURE, "[system]: sigaction SIGINT error ");

	if (0 != sigaction(SIGPIPE, &sig_cntrl, NULL))
			err(EXIT_FAILURE, "[system]: sigaction SIGQUIT error ");
			
	if (0 != sigaction(SIGTERM, &sig_cntrl, NULL))
			err(EXIT_FAILURE, "[system]: sigaction SIGQUIT error ");	
			
	if (0 != sigaction(SIGCHLD, &sig_cntrl, NULL))
			err(EXIT_FAILURE, "[system]: sigaction SIGQUIT error ");	
}



int close_all_live_fd(ssize_t *db_num, ssize_t *db_idx,  size_t num_limit, size_t *idx_limit, size_t *idx){	
	if (NULL == db_num){ 
		fprintf(stderr, "[server]; close_all_live_fd,  db_num is NULL.\n"); 
		return -1;
	}	
	if (NULL == db_idx){ 
		fprintf(stderr, "[server]; close_all_live_fd,  db_idx is NULL.\n"); 
		return -1;
	}	
	if (0 == num_limit){ 
		fprintf(stderr, "[server]; close_all_live_fd,  num_limit is 0.\n"); 
		return -1;
	}
	if (0 == idx_limit){ 
		fprintf(stderr, "[server]; close_all_live_fd,  idx_limit is 0.\n"); 
		return -1;
	}
	

	for (int i = 0; i < *idx; i++){
		if (-1 == del_el_db_num(db_num, db_idx, num_limit, idx_limit, idx, db_idx[i]))
			return -1;
	}
}



void set_tty_settings(struct termios *prev_settings){
    struct termios t = {0};
    if (-1 == tcgetattr(STDIN_FILENO, &t)){
		err(EXIT_FAILURE, "[system]: tcgetattr, ");
	}

    if (NULL != prev_settings)
        *prev_settings = t;

    t.c_lflag &= ~(ICANON | ISIG | IEXTEN | ECHO);
    t.c_iflag &= ~(BRKINT | IGNBRK);
    t.c_cc[VMIN] = 1;                   
    t.c_cc[VTIME] = 0;                 

    if (-1 == tcsetattr(STDIN_FILENO, TCSAFLUSH, &t))
        err(EXIT_FAILURE, "[system]: tcsetattr, ");
}




void write_common_console_msg(char *msg, struct winsize *wbuf, size_t *idx, size_t ln, int *fl_line_feed, int *total_usr_lines, char *out_buf_console){
	if (NULL == msg){
		fprintf(stderr, "[system] write_common_console_msg, msg is NULL.\n");
		exit(1);
	}
	if (NULL == wbuf){
		fprintf(stderr, "[system] write_common_console_msg, wbuf is NULL.\n");
		exit(1);
	}
	if (NULL == idx){
		fprintf(stderr, "[system] write_common_console_msg, wbuf is NULL.\n");
		exit(1);
	}
	if (NULL == fl_line_feed){
		fprintf(stderr, "[system] write_common_console_msg, wbuf is NULL.\n");
		exit(1);
	}
	if (NULL == total_usr_lines){
		fprintf(stderr, "[system] write_common_console_msg, wbuf is NULL.\n");
		exit(1);
	} 	
	if (NULL == out_buf_console){
		fprintf(stderr, "[system] write_common_console_msg, wbuf is NULL.\n");
		exit(1);
	} 	
	
	
	
	
	if (-1 == ioctl(STDIN_FILENO, TIOCGWINSZ, wbuf))
		err(EXIT_FAILURE, "error 1ioctl winsize: ");
		
		
	if (*total_usr_lines){
		write_n(STDOUT_FILENO, "\r", 1);
		write_n(STDOUT_FILENO, "\033[K", 3);

		for (int i = 0; i < *total_usr_lines ; i++){
			write_n(STDOUT_FILENO, "\033M", 2);			
			write_n(STDOUT_FILENO, "\033[K", 3);			
		}
	} else {
		write_n(STDOUT_FILENO, "\r>:", 3);
		write_n(STDOUT_FILENO, "\033[K", 3);		
	}
	*total_usr_lines = 0;
	
	write_n(STDOUT_FILENO, "\0338", 2);	

	if (!ln){			
		write_n(STDOUT_FILENO, msg, sizeof(msg)-1);	
			
	} else { 
		write_n(STDOUT_FILENO, msg, ln);				
	}
	
	write_n(STDOUT_FILENO, "\0337", 2);	
	
	if (*fl_line_feed){	
		printf("\033[%d;%dH", wbuf->ws_row, 0);
		write_n(STDOUT_FILENO, "\r>:", 3);			
		
		(*fl_line_feed)--;
	
	} else {
		printf("\033[?25l");
		write_n(STDOUT_FILENO, "\033[?25l", 6);	
		
		printf("\033[%d;%dH", wbuf->ws_row, 0);
		write_n(STDOUT_FILENO, "\r>:", 3);	
		
		write(STDOUT_FILENO, out_buf_console, *idx);
		write_n(STDOUT_FILENO, "\033[?25h", 6);	
	}
}