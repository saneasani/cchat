

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
		write_msg(cur_event_info->fd, "[server] Message is too long.\n", strlen("[server] Message is too long.\n"), 1, 0, 's');
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
	
	res1 = write_server_msg(cur_event_info->private->fd, buf_dst, buf_ln, 1, 0, 's');		
	res2 = write_server_msg(cur_event_info->fd, buf_dst, buf_ln, 1, 0, 's');	

	for (int i = 0; i < MAX_LINE; buf_dst[i++] = '\0');
																							
	if (-1 == res1 || -1 == res2)
		return -1;
	
	return 1;
}


void write_global_msg(event_info *cur_event_info, ssize_t db_gl_chat_idx[], size_t gl_idx){
	char tmp_buf[MAX_LINE] = "[%s]: ";
	char buf_dst[MAX_LINE] = {0};	
	size_t buf_ln = 6;

	
	int n = -1;
	if (-1 == (n = snprintf(buf_dst, MAX_LINE, tmp_buf, cur_event_info->name)))
		err(EXIT_FAILURE, "write_global_msg, ");	
	
	
	buf_ln += (size_t)n;
	buf_ln += cur_event_info->msg_chat_idx;
	
	if ((buf_ln) >= MAX_LINE){
		write_msg(cur_event_info->fd, "[server] Message is too long.\n", strlen("[server] Message is too long.\n"), 1, 0, 's');
		fprintf(stderr, "[system] write_global_msg, length >= MAX_EVENTS.\n");
		exit(1);
	}

	strncat(buf_dst, cur_event_info->msg_chat_buf, cur_event_info->msg_chat_idx);			
	printf("buf_dst[buf_ln] =>:%d\n", buf_dst[buf_ln]);
	
 	if ('\0' != buf_dst[buf_ln]){
		fprintf(stderr, "[system]: write_global_msg not null terminating.\n");
		exit(1);
	}


	for (int i = 0; i < gl_idx; i++){
		write_msg(db_gl_chat_idx[i], buf_dst, buf_ln, 1, 0, 's');
	} 
	
	
	for (int i = 0; i < MAX_LINE; buf_dst[i++] = '\0');			
	
	return;
}



int write_msg(int fd,  void *ptr_data, size_t in_len, int msg_type, int notif, char msg_struct){
	if (0 > fd){
		fprintf(stderr, "[system]: write_msg error, fd < 0.\n");
		exit(1);
	}
	if (in_len > CHUNK_SIZE){ 
		fprintf(stderr, "[system]: write_msg error, len(%d) > %d.\n", in_len , CHUNK_SIZE);
		exit(1);
	}
	if (msg_type < 1 || msg_type > 3){ 
		fprintf(stderr, "[system]: write_msg error, unsupported msg type.\n"); 
		exit(1);
	}
	
	if ('s' != msg_struct && 'n' != msg_struct){ 
		fprintf(stderr, "[system]: write_msg error, unsupported msg struct: %c\n", msg_struct); 
		exit(1);
	}

	char *ptr = NULL;
	size_t total_len = 0;
	struct message msg_std = {0};
	struct message_numbers msg_num = {0};
	

	if ('s' == msg_struct){	
		msg_std.msg_type = msg_type;
		if (notif) msg_std.notif = notif;
		else msg_std.notif= 0;
		
		char *data = ptr_data;		
		memcpy(msg_std.buf, data, in_len);
		
		if (2 == msg_type){
			msg_std.len = in_len;
		} else {
			msg_std.len = strnlen(msg_std.buf, in_len);
		}
		
		
		size_t tmp_len = 0;
		tmp_len = msg_std.len;
		msg_std.len = htons(msg_std.len);
		
		total_len = sizeof(msg_std.msg_type) + sizeof(msg_std.notif) + sizeof(msg_std.len) + tmp_len;
		

		ptr = (char*)&msg_std;
	}	

	else if ('n' == msg_struct) {	
		msg_num.msg_type = msg_type;
		if (notif) msg_num.notif = notif;
		else msg_num.notif= 0;
		
		msg_num.reserved = 0;
		msg_num.num_data = htons((*(uint32_t*)ptr_data));
	
		total_len = sizeof(msg_num.msg_type) + sizeof(msg_num.notif) + sizeof(msg_num.reserved) + sizeof(msg_num.num_data);

		ptr = (char*)&msg_num;	
	}	
	

	size_t n_left = total_len;
	ssize_t n_written = -1;

	size_t n_tmp = 0;
	 
	while (n_left > 0) {
		if ((n_written = write(fd, ptr, n_left)) <= 0) {
			if (n_written < 0 && (EINTR == errno || EAGAIN == errno || EWOULDBLOCK == errno))
				n_written = 0;		
				
			else if (EPIPE == errno) 
				return -1;
			else if (ECONNRESET == errno) 
				return -1;
			else 
				err(EXIT_FAILURE, "[server]: write msg error ");	
		}
		n_left -= n_written;
		ptr   += n_written;
		
	}
	
	
	if (total_len != n_written){
		fprintf(stderr, "[system]: write msg error: not all data is written.\n"); 
		exit(1);
	}
	
	return 1;
}


int write_server_msg(int fd, void *ptr_data, size_t in_len, int msg_type, int notif, char msg_struct){
	if (-1 == write_msg(fd, ptr_data, in_len, msg_type, notif, msg_struct))
		return -1;
	return 1;
}

void write_client_msg(int fd, void *ptr_data, size_t in_len, int msg_type, int notif, char msg_struct){
	if (-1 == write_msg(fd, ptr_data, in_len, msg_type, notif, msg_struct))
		exit(1);
}


char *sock_ntop(const struct sockaddr *sa, socklen_t salen){
    char portstr[8] = {0};
    static char str[128];		/* Unix domain is largest */

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
    if ((conn_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err(EXIT_FAILURE, "[system]: socket error ");
		
	if (connect(conn_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		err(EXIT_FAILURE, "[system]: connect error ");

	return conn_fd;
}


