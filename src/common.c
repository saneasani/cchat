
#include "common.h"



 
 void write_n(int fd, char *msg, size_t ln){
	if (fd < 0){
		fprintf(stderr, "[system]: write_n, fd < 0.\n");
		exit(1);
	}
	if (NULL == msg){
		fprintf(stderr, "[system]: write_n, msg is NULL.\n");
		exit(1);
	}
	
	ssize_t cur_len = 0;
	ssize_t total_len = (ssize_t)ln;
	ssize_t n_left = (ssize_t)ln;
	ssize_t n_written = -1;	
	char *ptr = msg;

	 
	while (n_left > 0) {
		if ((n_written = write(fd, ptr, n_left)) <= 0) {
			if (n_written < 0 && (EINTR == errno || EAGAIN == errno || EWOULDBLOCK == errno))
				n_written = 0;

			else if (0 == n_written){
				fprintf(stderr, "[system]: write_n error: 0 is written.\n");
				break;
			}
			else 
				err(EXIT_FAILURE, "[system]: write_n error ");	
		}
		n_left -= n_written;
		ptr   += n_written;
		cur_len += n_written;
	}
	
 
	
	if (total_len != cur_len){
		fprintf(stderr, "[system]: write_n error: not all data is written.\n"); 
		exit(1);
	}
	
	
}


void write_to_event_fd(int fd){
	if (fd < 0){
		fprintf(stderr, "[system]: write_to_event_fd, fd < 0.\n");
		exit(1);
	}
	uint64_t u = 1;
	ssize_t s = -1;
	s = write(fd, &u, UINT64_T_SZ);
	if (s != UINT64_T_SZ)
		err(EXIT_FAILURE, "[system]: write_to_event_fd error ");			
}


void fun_on_exit(int status, void *arg){
	struct signal_struct *sig_strct = arg;
	
	if (1 == *sig_strct->sig_occured){
		*sig_strct->sig_occured = 0;
		
		if (NULL != sig_strct->prev_settings){	
			if (-1 == tcsetattr(STDIN_FILENO, TCSAFLUSH, sig_strct->prev_settings))
				err(EXIT_FAILURE, "[system]: tcgetattr, ");
			fprintf(stderr, "[system]: Shutting down client. Settinng back tty settings. %d\n", status);
		
		} else {
			fprintf(stderr, "[system]: Error on restoring a tty settings. %d\n", status);
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
	 
	 
	 
 

int push_neg_num_to_end_n_reduce_idx(num_db *inst_db, size_t nums_size) {
	if (NULL == inst_db){ 
		fprintf(stderr, "[system]: push_neg_num_to_end_n_reduce_id, inst_db is NULL.\n");
		exit(1);
	}
	if (0 == nums_size){ 
		fprintf(stderr, "[system]: push_neg_num_to_end_n_reduce_id, nums_size is 0.\n");
		exit(1);
	}
	
    ssize_t *cur = inst_db->idx_db, *main = inst_db->idx_db, *e = inst_db->idx_db + nums_size;
    size_t t = -1;

    for(;cur<=e;cur++){
        if(-1 != *main && main <= cur) {
			++main;
        }			
        else if(-1 == *main){  
            if(-1 != *cur){  
                inst_db->fd_db[*cur] = main - inst_db->idx_db;
                t = *main;
                *main++ = *cur;
                *cur = t;
            }
        }      
    }
	return 1;
}


void put_el_db_num(num_db *inst_db, int num){
	if (NULL == inst_db){ 
		fprintf(stderr, "[system]: put_el_db_num, inst_db is null.\n"); 
		exit(1);
	}
	if (num < 0){ 
		fprintf(stderr, "[system]: put_el_db_num, num < 0.\n");
		exit(1);
	}

	size_t tmp_num = (size_t)num;
 
	if (tmp_num < inst_db->fd_limit && -1 != inst_db->fd_db[tmp_num]){ 
		fprintf(stderr, "[system]: put_el_db_num, num in db_num: %ld\n", tmp_num);
		exit(1);
	}


    size_t tmp_fd_limit = inst_db->fd_limit;
    
	while (inst_db->fd_limit <= tmp_num){
	    if (0 == inst_db->fd_limit || 1 == inst_db->fd_limit){
	        inst_db->fd_limit = 2;
	    } else {
	         inst_db->fd_limit *= inst_db->fd_limit;
	    }
		if (inst_db->fd_limit > tmp_num){
			break;
		}
	}
	
	ssize_t *tmp_fd_db = inst_db->fd_db;
	if (NULL == (tmp_fd_db = erealloc(inst_db->fd_db, inst_db->fd_limit * sizeof(ssize_t)))){
	    exit(1);
	} else {
        for (size_t i = tmp_fd_limit; i < inst_db->fd_limit; tmp_fd_db[i++] = -1);
        inst_db->fd_db = tmp_fd_db;
	}
	 
	
	size_t tmp_idx_limit = inst_db->idx_num_limit;
	
	while (inst_db->idx_num_limit <= inst_db->idx_num){
	    if (0 == inst_db->idx_num_limit || 1 == inst_db->idx_num_limit){
	        inst_db->idx_num_limit = 2;
	    } else {
	        inst_db->idx_num_limit *= inst_db->idx_num_limit;
	    }
		if (inst_db->idx_num_limit > inst_db->idx_num){
			break;
		}
		
	} 
 	
		
	ssize_t *tmp_idx_db = inst_db->idx_db;
	
	if (NULL == (tmp_idx_db = erealloc(inst_db->idx_db, inst_db->idx_num_limit * sizeof(ssize_t)))){
	    exit(1);
	}else{
        for (size_t i = tmp_idx_limit; i < inst_db->idx_num_limit; tmp_idx_db[i++] = -1);
        inst_db->idx_db = tmp_idx_db;
	}
	
	inst_db->idx_db[inst_db->idx_num] = tmp_num;
	inst_db->fd_db[tmp_num] = inst_db->idx_num;
	inst_db->idx_num++;
}
 
 


int del_el_db_num(num_db *inst_db, int num){
	if (NULL == inst_db){ 
		fprintf(stderr, "[system]: del_el_db_num, inst_db is NULL.\n"); 
		return -1;
	}	
	if (num < 0){ 
		fprintf(stderr, "[system]: del_el_db_num, num < 0.\n");
		return -1;
	}

	if (-1 == inst_db->fd_db[num]){ 
		fprintf(stderr, "[system]: num is not in fd_db, del_el_db_num. %d\n", num);
		return -1;
	}
	
	
	size_t i = inst_db->fd_db[num];	
	inst_db->idx_db[i] = -1;
	
	if ((inst_db->idx_num-1 != i)){
		if (-1 == push_neg_num_to_end_n_reduce_idx(inst_db, inst_db->idx_num_limit - 1))
			return -1;
	}
	
	inst_db->fd_db[num] = -1;
	inst_db->idx_num--;
	
	return 1;
}

	
	
void put_ptr_db_num(ptr_db *inst_db, event_info* ev_inf){	
	if (NULL == inst_db){ 
		fprintf(stderr, "[system]: put_el_db_num, recevied null in db_num.\n"); 
		exit(1);
	}
	
	size_t tmp_num = (size_t)ev_inf->fd;

	if (tmp_num < inst_db->fd_limit && -1 != inst_db->fd_db[tmp_num]){ 
		fprintf(stderr, "[system]: num is already in db_num, put_el_db_num: %ld : %ld\n", tmp_num, inst_db->fd_db[tmp_num]);
		exit(1);
	}

	size_t tmp_fd_limit = inst_db->fd_limit;
	
	while (inst_db->fd_limit <= tmp_num){	   
		if (0 == tmp_num || 1 == inst_db->fd_limit){
			inst_db->fd_limit = 2;			
		} else {
			inst_db->fd_limit *= inst_db->fd_limit;
		}
		if (inst_db->fd_limit > tmp_num){
			break;
		}
	} 	
	
		          
	ssize_t *tmp_fd_db = inst_db->fd_db;
	
	if (NULL == (tmp_fd_db = erealloc(inst_db->fd_db, inst_db->fd_limit * sizeof(ssize_t)))){
			exit(1);
	} else {
		for (size_t i = tmp_fd_limit; i < inst_db->fd_limit; tmp_fd_db[i++] = -1);
		inst_db->fd_db = tmp_fd_db;
	}
			
	 
	
	size_t tmp_ptr_limit = inst_db->idx_ptr_limit;	
	
	while (inst_db->idx_ptr_limit <= inst_db->idx_ptr){
		if (0 == inst_db->idx_ptr || 1 == inst_db->idx_ptr_limit){
		   inst_db->idx_ptr_limit = 2;  		
		} else {
			inst_db->idx_ptr_limit *= inst_db->idx_ptr_limit; 		
		}
		
		if (inst_db->idx_ptr_limit > inst_db->idx_ptr){
			break;
		}
	}
	
		
	event_info **tmp_idx_db = inst_db->idx_db;

	if (NULL == (tmp_idx_db = erealloc(inst_db->idx_db, inst_db->idx_ptr_limit * sizeof(event_info *)))){
		exit(1);
	} else {
		for (size_t i = tmp_ptr_limit; i < inst_db->idx_ptr_limit; tmp_idx_db[i++] = NULL);
		inst_db->idx_db = tmp_idx_db;
	}           
		
	 
	inst_db->idx_db[inst_db->idx_ptr] = ev_inf;
	inst_db->fd_db[tmp_num] = inst_db->idx_ptr;
	inst_db->idx_ptr++;
}




int push_null_to_end_n_reduce_idx(ptr_db *inst_db, size_t nums_size) {
	if (NULL == inst_db){ 
		fprintf(stderr, "[system]: push null, db_idx is NULL.\n");
		exit(1);
	}
	if (0 == nums_size){ 
		fprintf(stderr, "[system]: push null, nums_size is 0.\n");
		exit(1);
	}

    event_info **cur = inst_db->idx_db, **main = inst_db->idx_db, **e = inst_db->idx_db + nums_size;
    event_info *t = NULL;


    for(;cur<=e;cur++){
        if(NULL != *main && *main <= *cur ) {
                        ++main;
        }
        else if(NULL == *main){  
            if(NULL != *cur){
                inst_db->fd_db[(*cur)->fd] = main - inst_db->idx_db;
                t = *main;
                *main++ = *cur;
                *cur = t;
            }
        }      
    }
	return 1;
}



 



int del_ptr_db_num(ptr_db *inst_db, event_info *ev_inf){
	if (NULL == inst_db){ 
			fprintf(stderr, "[system]: del_el_db_num, inst_db is NULL.\n"); 
			return -1;
	}
	if (NULL == ev_inf){ 
			fprintf(stderr, "[system]: del_el_db_num, ev_inf is NULL.\n");
			return -1;
	}
	if (-1 == inst_db->fd_db[ev_inf->fd]){ 
			fprintf(stderr, "[system]: num is not in db_num, del_el_db_num.\n");
			return -1;
	}

	size_t tmp_num = (size_t)ev_inf->fd;

	size_t i = inst_db->fd_db[tmp_num];
	
	inst_db->idx_db[i] = NULL;

	if (((inst_db->idx_ptr)-1 != i)){
		if (-1 == push_null_to_end_n_reduce_idx(inst_db, inst_db->idx_ptr_limit - 1))
			return -1;
	}

	inst_db->fd_db[tmp_num] = -1;
	inst_db->idx_ptr--;


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
	cur_event_info->file_offset = 0;
	cur_event_info->rem_file = 0;
	

	
	if ('s' == cl_or_sr){
		cur_event_info->file->file = NULL;
		cur_event_info->file = NULL;
		
	}
	
	for (int i = 0; i < MAX_LINE; cur_event_info->file_name[i++] = 0);		
	for (int i = 0; i < CHUNK_SIZE; cur_event_info->file_chunk[i++] = 0);	
}																


void clear_client(ptr_db *global_users_db, ptr_db *ep_db, control_nodes *cn, event_info **node, hash_map *map){
	
	
	printf("[system][clear client] name =>:%s\n", (*node)->name);
	printf("[system][clear client] fd =>:%d\n", (*node)->fd);
	
	if (-1 != ep_db->fd_db[(*node)->fd]){
		del_ptr_db_num(ep_db, *node);
	}	
	del_ptr_db_num(global_users_db, *node);
	clear_entry_by_idx_fd(map, (*node)->entry_idx, (*node)->fd);

	if (-1 == close((*node)->fd)){
		err(EXIT_FAILURE,  "[server]: clear_client: close fd error.\n");
	}
	pop_node(cn, *node);
	*node = NULL;
	
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



int close_all_live_fd(num_db *db_num){	
	if (NULL == db_num){ 
		fprintf(stderr, "[server]: close_all_live_fd,  db_num is NULL.\n"); 
		return -1;
	}	

	for (size_t i = 0; i < db_num->idx_num; i++){
		if (-1 == del_el_db_num(db_num, db_num->idx_db[i]))
			return -1;
	}
	return 1;
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
		fprintf(stderr, "[system] write_common_console_msg, idx is NULL.\n");
		exit(1);
	}
	if (NULL == fl_line_feed){
		fprintf(stderr, "[system] write_common_console_msg, fl_line_feed is NULL.\n");
		exit(1);
	}
	if (NULL == total_usr_lines){
		fprintf(stderr, "[system] write_common_console_msg, total_usr_lines is NULL.\n");
		exit(1);
	} 	
	if (NULL == out_buf_console){
		fprintf(stderr, "[system] write_common_console_msg, out_buf_console is NULL.\n");
		exit(1);
	} 	
	
	
	
	
	if (-1 == ioctl(STDIN_FILENO, TIOCGWINSZ, wbuf))
		err(EXIT_FAILURE, "[system[ error 1ioctl winsize: ");
		
		
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


void init_ptr_db(ptr_db *instance_db){
	if (NULL == instance_db){
		fprintf(stderr, "[system]: init_ptr_db, instance_db is NULL.\n");
		exit(1);
	}
	
	instance_db->fd_limit = 100;
	instance_db->fd_db = ecalloc(instance_db->fd_limit, sizeof(ssize_t));
	for (size_t i = 0; i < instance_db->fd_limit; i++) instance_db->fd_db[i] = -1;
	
	instance_db->idx_ptr_limit = instance_db->fd_limit/2;
	instance_db->idx_ptr = 0;	
	instance_db->idx_db = ecalloc(instance_db->idx_ptr_limit, sizeof(event_info*));
	for (size_t i = 0; i < instance_db->idx_ptr_limit; i++) instance_db->idx_db[i] = NULL;	
}



void init_num_db(num_db *instance_db){	
	if (NULL == instance_db){
		fprintf(stderr, "[system]: init_ptr_db, instance_db is NULL.\n");
		exit(1);
	}	
	
	instance_db->fd_limit = 100;
	instance_db->fd_db = ecalloc(instance_db->fd_limit, sizeof(ssize_t));
	for (size_t i = 0; i < instance_db->fd_limit; i++) instance_db->fd_db[i] = -1;
	
	instance_db->idx_num_limit = instance_db->fd_limit/2;
	instance_db->idx_num = 0;	
	instance_db->idx_db = ecalloc(instance_db->idx_num_limit, sizeof(ssize_t));
	for (size_t i = 0; i < instance_db->idx_num_limit; i++) instance_db->idx_db[i] = -1;		
}
