#ifndef H_CONNECTIONS
#define H_CONNECTIONS

#include "tools.h"
#include "events.h"
#include "common.h"


#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <termios.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>





#define MAX_LINE 1026
#define CHUNK_SIZE 1030


int write_private_msg(event_info *cur_event_info);
void write_global_msg(event_info *cur_event_info, ptr_db *gl_users_db);
int write_msg(event_info *cur_event_info,  void *ptr_data, size_t in_len, int msg_type, int notif, char msg_struct);
int write_server_msg(event_info *cur_event_info, void *ptr_data, size_t in_len, int msg_type, int notif, char msg_struct);
void write_client_msg(int fd, int *lock_fd, event_info *cur_event_info, void *ptr_data, size_t in_len, int msg_type, int notif, char msg_struct);
char *sock_ntop(const struct sockaddr *sa);
int create_tcp_listen_socket(const char *port);
int create_tcp_client_socket(char *addr, char *port);
char parse_command(event_info *ev_info, char *buf, size_t ln, char  **ptr_name, int *status, struct winsize *wbuf, size_t *idx, int *fl_line_feed, int *total_usr_lines);
int read_incoming_data(ssize_t *n_read, ssize_t *in_idx, char buf_in[], event_info* data_usr);

#endif