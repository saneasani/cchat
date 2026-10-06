#ifndef H_COMMON
#define H_COMMON


#define _GNU_SOURCE
#define _XOPEN_SOURCE 600

#include "tools.h"
#include "events.h"
#include "entry_map.h"
#include "connections.h"



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


struct signal_struct{
	volatile sig_atomic_t *sig_occured;
	struct termios *prev_settings;
};

void write_n(int fd, char *msg, size_t ln);
void write_to_event_fd(uint64_t *u, int fd);
void fun_on_exit(int exit_status, void *arg);
int filter_command(char command, char *buf, char *comparison_buf, size_t i, size_t *j, char *name, int *flag_name, int *count_ok, int **status);
char parse_command(int fd, char *buf, size_t ln, char  **ptr_name, int *status, struct winsize *wbuf, size_t *idx, int *fl_line_feed, int *total_usr_lines);
int read_incoming_data(ssize_t *n_read, size_t *in_idx, char buf_in[], event_info* data_usr);
int push_neg_num_to_end_n_reduce_idx(ssize_t *db_idx, ssize_t *db_fd, size_t nums_size);
void put_el_db_num(ssize_t **db_num, ssize_t **db_idx, size_t *num_limit, size_t *idx_limit, size_t *idx, int num);
int del_el_db_num(ssize_t *db_num, ssize_t *db_idx, size_t num_limit, size_t *idx_limit, size_t *idx, int num);
void clear_msg_buf(event_info *cur_event_info);
void clear_file_send(event_info *cur_event_info, int fl, int cl_or_sr);
void clear_client(size_t *db_gl_chat_fd, size_t *db_gl_chat_idx, size_t gl_fd_limit, size_t *gl_idx_limit, size_t *idx, size_t *gl_total_usrs, 
				  control_nodes *cn, event_info **node, hash_map *map);
int pty_master_open(char *slave_name, size_t sn_len);
pid_t pty_fork(int *master_fd, char *slave_name, size_t sn_len,
               const struct termios *slave_termios, const struct winsize *slave_ws);
void set_non_block_buf_in_out_err();
void set_non_block_fd(int fd);
void wait_children();
void catcher(int sig_num);
void set_signals();
int close_all_live_fd(ssize_t *db_num, ssize_t *db_idx,  size_t num_limit, size_t *idx_limit, size_t *idx);
void set_tty_settings(struct termios *prev_settings);
void write_common_console_msg(char *msg, struct winsize *wbuf, size_t *idx, size_t ln, int *fl_line_feed, int *total_usr_lines, char *out_buf_console);


#endif