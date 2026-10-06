#ifndef H_EVENTS
#define H_EVENTS

#include "tools.h"

#include <err.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/epoll.h>

#define MAX_LINE 1026
#define CHUNK_SIZE 1030
#define NODE_QUANTITY 100




typedef struct event_info_{
	int fd;
	char name[10];
	int init_name;
	ssize_t entry_idx;
	int global;

	int fl_notif_rcv;
	int notif_rcv;
	uint8_t type_msg;	
	int start_msg;
	int flag_len_msg;
	
	char msg_chat_buf[MAX_LINE];
	char len_chat_msg_arr[2];
	uint16_t len_chat_msg;
	size_t msg_chat_idx;
	
	
	// --- private ---
	struct event_info_ *private;
	int init_pr;
	
 
	// --- shell ---
	int init_dst_shell;
	int init_src_shell;
	
	struct event_info_ *shell;
	

	// --- file ---
	int fl_file_src_init;
	int fl_file_dst_init;
	
	int fl_file_name;
	int fl_file_size;
	int fl_file_chunks;
	int file_wr_fd;
	int file_read_fd;

	

	int file_read_send_fd;
	off_t file_offset;

	
	size_t msg_file_idx;
	size_t msg_num_idx;
	
	uint16_t reserved;
	int flag_reserved_msg;
	char len_reserved_arr[2];
	
	uint32_t num_data;
	char num_data_arr[4];
	
	char file_name[MAX_LINE];
	size_t name_file_size;
	size_t file_size;
	size_t left_file_size;
	size_t offset_file;
	size_t rem_file;
	char file_chunk[CHUNK_SIZE];

	struct event_info_ *file;
	
	
	// --- timer ---
	struct itimerspec ts;

	int timer_rq_db_p[2];
	int timer_rq_db_f[2];
	int timer_rq_db_s[2];
	int cur_rq;
	int limit_rq;
	

	struct event_info_ *next;
	struct event_info_ *prev;

} event_info;



typedef struct{
	event_info *head;
	event_info *free_node_head;
	event_info *allocated_node;
	size_t size_left;

} control_nodes;



typedef struct control_blocks_{
	event_info *alloc_block;
	struct control_blocks_ *next;

} control_blocks;


control_nodes *create_cn();
control_blocks *create_bl();
event_info *get_new_node(control_nodes *cn, control_blocks **cb_head);
event_info *push_node(control_nodes *cn, int fd, control_blocks **cb);
void pop_node(control_nodes *cn, event_info *node);
void return_node(control_nodes *cn, event_info *node);
int add_event(int epoll_fd, int fd, control_nodes *cn, control_blocks **cb, int action);
event_info *get_head(control_nodes *cn);
void clear_blocks(control_blocks **cb_head);
int create_epoll();


#endif