#ifndef H_CONNECTIONS
#define H_CONNECTIONS

#include "tools.h"
#include "events.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_LINE 1026
#define CHUNK_SIZE 1030


struct message{
	uint8_t msg_type;
	uint8_t notif;
	uint16_t len;
	char buf[MAX_LINE];
};


struct message_numbers{
	uint8_t msg_type;
	uint8_t notif;
	int16_t reserved;
	uint32_t num_data;
};


int write_private_msg(event_info *cur_event_info);
void write_global_msg(event_info *cur_event_info, ssize_t db_gl_chat_idx[], size_t gl_idx);
int write_msg(int fd,  void *ptr_data, size_t in_len, int msg_type, int notif, char msg_struct);
int write_server_msg(int fd, void *ptr_data, size_t in_len, int msg_type, int notif, char msg_struct);
void write_client_msg(int fd, void *ptr_data, size_t in_len, int msg_type, int notif, char msg_struct);
char *sock_ntop(const struct sockaddr *sa, socklen_t salen);
int create_tcp_listen_socket(const char *port);
int create_tcp_client_socket(char *addr, char *port);


#endif