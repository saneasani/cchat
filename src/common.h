#ifndef H_COMMON
#define H_COMMON


#define _GNU_SOURCE
#define _XOPEN_SOURCE 600

#include "tools.h"
#include "events.h"
#include "entry_map.h"




#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <fcntl.h>

#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#define MAX_LINE 1026
#define MAX_SNAME 1000
#define CHUNK_SIZE 1030
#define UINT64_T_SZ sizeof(uint64_t)


typedef struct{
	size_t fd_limit;
	ssize_t *fd_db;
	
	size_t idx_num_limit;
	size_t idx_num;	
	ssize_t *idx_db;
	
} num_db;

typedef struct{
	size_t fd_limit;
	ssize_t *fd_db;
	
	size_t idx_ptr_limit;
	size_t idx_ptr;	
	event_info **idx_db;
	
} ptr_db;



struct signal_struct{
	volatile sig_atomic_t *sig_occured;
	struct termios *prev_settings;
};

 
void init_ptr_db(ptr_db *instance_db);
void init_num_db(num_db *instance_db);

void put_ptr_db_num(ptr_db *inst_db, event_info* ev_inf);	
int push_null_to_end_n_reduce_idx(ptr_db *inst_db, size_t nums_size);
int del_ptr_db_num(ptr_db *inst_db, event_info *ev_inf);

void put_el_db_num(num_db *inst_db, int num);
int push_neg_num_to_end_n_reduce_idx(num_db *inst_db, size_t nums_size);
int del_el_db_num(num_db *inst_db, int num);	


void write_n(int fd, char *msg, size_t ln);
void write_to_event_fd(int fd);
void fun_on_exit(int exit_status, void *arg);

int filter_command(char command, char *buf, char *comparison_buf, size_t i, size_t *j, char *name, int *flag_name, int *count_ok, int **status);



void clear_msg_buf(event_info *cur_event_info);
void clear_file_send(event_info *cur_event_info, int fl, int cl_or_sr);				  
void clear_client(ptr_db *global_users_db, ptr_db *ep_db, control_nodes *cn, event_info **node, hash_map *map);			  
				  
				  
int pty_master_open(char *slave_name, size_t sn_len);
pid_t pty_fork(int *master_fd, char *slave_name, size_t sn_len,
               const struct termios *slave_termios, const struct winsize *slave_ws);
void set_non_block_buf_in_out_err();
void set_non_block_fd(int fd);
void wait_children();
void catcher(int sig_num);
void set_signals();
int close_all_live_fd(num_db *db_num);
void set_tty_settings(struct termios *prev_settings);
void write_common_console_msg(char *msg, struct winsize *wbuf, size_t *idx, size_t ln, int *fl_line_feed, int *total_usr_lines, char *out_buf_console);

#endif