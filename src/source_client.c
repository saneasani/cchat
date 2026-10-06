

#include "tools.h"
#include "events.h"
#include "common.h"
#include "connections.h"

#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/eventfd.h>

#define MAX_EVENTS 1000


int main(int argc, char **argv){
    if (argc != 3){
        fprintf(stderr, "Usage: <Program name> <IP address> <Port number>.\n");
		exit(1);
	}

	// --- signals ---
	set_signals();

	// --- non blocking buf---
	set_non_block_buf_in_out_err();
	
	// --- store all live fds ---
	size_t live_fd_limit = 100;
	size_t live_fd_idx_limit = live_fd_limit/2;
	size_t live_idx = 0;

	ssize_t *live_fd = ecalloc(live_fd_limit, sizeof(ssize_t));
	for (size_t i = 0; i < live_fd_limit; i++) live_fd[i] = -1;
	ssize_t *live_fd_idx = ecalloc(live_fd_idx_limit, sizeof(ssize_t));
	for (size_t i = 0; i < live_fd_idx_limit; i++) live_fd_idx[i] = -1;
		
		
    // --- socket ---
	int conn_fd = create_tcp_client_socket(argv[1], argv[2]);
	put_el_db_num(&live_fd, &live_fd_idx, &live_fd_limit, &live_fd_idx_limit, &live_idx , conn_fd);


	// --- tty settings ---
	struct termios tty_settings = {0};
	set_tty_settings(&tty_settings);
	
	struct signal_struct sig_struct = {0};
	sig_struct.prev_settings = &tty_settings;
	
	if (MAP_FAILED == (sig_struct.sig_occured = mmap(0, sizeof(sig_atomic_t), PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0)))
		err(EXIT_FAILURE, "[system]: mmap failed, ");
	*sig_struct.sig_occured = 1;
	
	
	if (0 != on_exit(fun_on_exit, &sig_struct))
		err(EXIT_FAILURE, "[system]: on_exit error, ");
	

	// --- for managing events blocks/nodes ---
	control_blocks *ctrl_bl_head = NULL;
	control_nodes *ctrl_nds = create_cn();	
	

	// --- init epoll ---
	struct epoll_event total_events[MAX_EVENTS] = {0};
	int epoll_fd = create_epoll();

	set_non_block_fd(conn_fd);
	add_event(epoll_fd, conn_fd, ctrl_nds, &ctrl_bl_head, 1);
	set_non_block_fd(STDIN_FILENO);
	add_event(epoll_fd, STDIN_FILENO, ctrl_nds, &ctrl_bl_head, 1);

	put_el_db_num(&live_fd, &live_fd_idx, &live_fd_limit, &live_fd_idx_limit, &live_idx , epoll_fd);


    // --- managing shell ---
	int master_fd_loc = -1;
	int master_fd = -1, script_fd = -1;
	int fl_shell_dst_init = 0;
	int fl_shell_src_init = 0;
	pid_t child_pid = -1;
	pid_t child_pid_loc = -1;
	

    // --- file sending ---
   	static char tmp_command_file_send[MAX_LINE];
	int event_file_fd = 0;
	uint64_t u = 0;
	size_t s = 0;
	
	
	// --- console ---
	static char in_buf_console[2];
	static char out_buf_console[MAX_LINE];
	
	size_t j_cnsl = 0, tmp_j_cnsl = 0;
	char el = 0;
	
	int fl_browse_active = 0, fl_file_wr = 0,
	fl_line_feed = 0;
	int total_lines = 0;
	int count_escape = 0;

	struct winsize wbuf = {0};
	if (-1 == ioctl(STDIN_FILENO, TIOCGWINSZ, &wbuf))
		err(EXIT_FAILURE, "error 2ioctl winsize: ");
	
	
	write_n(STDOUT_FILENO, "\033[2J\033[H", 7);
	write_n(STDOUT_FILENO, "\0337", 2);
	printf("\033[%d;1H", wbuf.ws_row);		
	write_n(STDOUT_FILENO, ">:", 2);


   	// --- common ---
	event_info *cur_event_info = NULL;
	event_info *cur_event_info_tmp = NULL;
	
	ssize_t n_read = 0;
	int num_fds = -1, n = 0;

	
	for (;;) {	
		if (-1 == (num_fds = epoll_wait(epoll_fd, total_events, MAX_EVENTS, -1))){
			if (-1 == num_fds){
				if (EINTR == errno)	
					continue;
				err(EXIT_FAILURE, "[system]: epoll_wait error: ");
			}
		}		
		
		char line[MAX_LINE] = {0};
		ssize_t ln = -1;
		
		for (n = 0; n < num_fds; n++){
			cur_event_info = (event_info*)total_events[n].data.ptr;
			
			if (total_events[n].events & (EPOLLERR| EPOLLHUP)){			
				int err_num = 0;
				socklen_t err_len = sizeof(err_num);
							
				write_common_console_msg("[system]: epoll error.\n", &wbuf, &j_cnsl, 23, &fl_line_feed, &total_lines, out_buf_console);	
				
				char err_buff[MAX_LINE] = {0};
				snprintf(err_buff, MAX_LINE, "[system]: error epoll fd = %s\n", strerror(err_num));
				write_common_console_msg(err_buff, &wbuf, &j_cnsl, strlen(err_buff), &fl_line_feed, &total_lines, out_buf_console);	
	
	
				if (0 == getsockopt(cur_event_info->fd, SOL_SOCKET, SO_ERROR, (void *)&err_num, &err_len)) {
					char err_sock_buff[MAX_LINE] = {0};
					snprintf(err_sock_buff, MAX_LINE, "[system]: error socket epoll num = %s\n", strerror(err_num));
					
					write_common_console_msg(err_sock_buff, &wbuf, &j_cnsl, strlen(err_sock_buff), &fl_line_feed, &total_lines, out_buf_console);					
				}			
				exit(1);			
			}		
						
			if (STDIN_FILENO == cur_event_info->fd){			
				for (;;){
					if (-1 == (n_read = read(STDIN_FILENO, in_buf_console, 1))){
						if (EAGAIN == errno || EWOULDBLOCK == errno){ 
							break;
						}
						else if (EINTR == errno) 
							continue;	
						else if (EIO == errno){
							exit(1);
						}
						else err(EXIT_FAILURE, "[system]: read client STDIN error: ");						
					}
					if (0 == n_read)
						exit(1);
					
					
					el = in_buf_console[0];
					
					if (j_cnsl < MAX_LINE){	
						if ('\e' == el){
							count_escape++;
							continue;
						}					
						else if (1 == count_escape && '[' == el){
							count_escape++;
							continue;
						}
						else if (2 == count_escape) {
							if ('A' == el || 'B' == el || 'C' == el || 'D' == el){
								count_escape = 0;
								continue;
							} else {
								static char escape_msg[] = "[system]: Unsupported command.\n"; 
								write_common_console_msg(escape_msg, &wbuf, &j_cnsl, 31, &fl_line_feed, &total_lines, out_buf_console);	
								exit(1);							
							}					
						}
						
						else if ('\t' == el){
							continue;
						}
												
						else if (127 == el){ 
							if (!j_cnsl)
								continue;
								
							j_cnsl--;
							out_buf_console[j_cnsl] = '\0';
											
							if (0 == ((j_cnsl+3) % wbuf.ws_col)){
								write_n(STDOUT_FILENO, "\033M", 2);						
								printf("\033[%dC", wbuf.ws_col);
								write_n(STDOUT_FILENO, " ", 1);	
								
							} else {
								write_n(STDOUT_FILENO, "\033[D", 3);	
								write_n(STDOUT_FILENO, " ", 1);
								write_n(STDOUT_FILENO, "\033[D", 3);

							}					
							continue;
						}
						
						else if ('\n' == el){
							fl_line_feed = 1;
							
							out_buf_console[j_cnsl++] = el;
							out_buf_console[j_cnsl] = '\0';
							
							int s_com = -1;
							char *ptr_name = NULL;
							char command = 0;
									
				
							if ('/' == out_buf_console[0] || 'e' == out_buf_console[0]) {
								command = parse_command(-1, out_buf_console, j_cnsl, &ptr_name, &s_com, &wbuf, &j_cnsl, &fl_line_feed,  &total_lines);
							}
							//printf("command =>:%c fl_shell_src_init=>%d\n", command, fl_shell_src_init);
	
							if ('n' == command) {}						
							else if ('c' == command){}
							
							else if ('e' == command && 1 == fl_file_wr){
								fl_file_wr--;
							}
								
							else if ('e' == command && 'e' == out_buf_console[0] && 1 == fl_browse_active){
								fl_browse_active--;
								if (-1 == del_el_db_num(live_fd, live_fd_idx, live_fd_limit, &live_fd_idx_limit, &live_idx, master_fd_loc))
									exit(1);	
								close(master_fd_loc);
								kill(child_pid_loc, SIGKILL);
							
							}
							else if ('e' == command && 1 == fl_shell_dst_init && 'e' == out_buf_console[0]){	
								write_client_msg(conn_fd, out_buf_console, j_cnsl, 3, 0, 's');
								fl_shell_dst_init--;
								kill(child_pid, SIGKILL);
								if (-1 == close(master_fd))
									err(EXIT_FAILURE, "[system] error closeing master_fd.\n");	
								del_el_db_num(live_fd, live_fd_idx, live_fd_limit, &live_fd_idx_limit, &live_idx, master_fd);								
							}
							
							else if ('e' == command && 1 == fl_shell_src_init && 'e' == out_buf_console[0]){	
								write_client_msg(conn_fd, out_buf_console, j_cnsl, 3, 0, 's');
								fl_shell_src_init--;
							}						
								 
							else if ('e' == command && 'e' == out_buf_console[1] && 0 == fl_browse_active){
								exit(0);
							}
								
										
							else if (fl_file_wr){
								while (1) {
									out_buf_console[j_cnsl-1] = '\0'; // cut '\n'
									
									if (-1 == (cur_event_info->file_wr_fd = open(out_buf_console, O_RDONLY))){
										if (EINTR == errno)
											continue;
											
										else if (EACCES == errno){
											static char file_err_acc_msg[] = "[system]: You don't have acces of the file.\n";
											write_common_console_msg(file_err_acc_msg, &wbuf, &j_cnsl, 44, &fl_line_feed, &total_lines, out_buf_console);																			
											break;
										}
										else if (ENOTDIR == errno){
											static char file_err_dir_msg[] = "[system]: The file is directory.\n";
											write_common_console_msg(file_err_dir_msg, &wbuf, &j_cnsl, 33, &fl_line_feed, &total_lines, out_buf_console);												
											break;
										}
										else if (ENOENT == errno){
											static char file_err_eno_msg[] = "[system]: The file doesn't exists.\n";
											write_common_console_msg(file_err_eno_msg, &wbuf, &j_cnsl, 35, &fl_line_feed, &total_lines, out_buf_console);																				
											break;
										}
	
										err(EXIT_FAILURE, "[system]: Error user file.");
									}	
									
									else {																			
										struct stat st = {0};
										off_t fl_size = -1;

										if (-1 == fstat(cur_event_info->file_wr_fd, &st))
											err(EXIT_FAILURE, "[system]: Error while getting file stat ");																						
										fl_size = st.st_size; 
										
										if (0 == fl_size){
											static char empty_file_msg[] = "[system]: The file is empty.\n";
											write_common_console_msg(empty_file_msg, &wbuf, &j_cnsl, 29, &fl_line_feed, &total_lines, out_buf_console);																				
											break;
										}
										if (fl_size < 0){
											static char neg_file_msg[] = "[system]: The file size is negative.\n";
											write_common_console_msg(neg_file_msg, &wbuf, &j_cnsl, 37, &fl_line_feed, &total_lines, out_buf_console);																				
											break;			
										}
								
								
										fl_file_wr--;							
		
										event_file_fd = eventfd(0, EFD_NONBLOCK);
										set_non_block_fd(event_file_fd);
										add_event(epoll_fd, event_file_fd, ctrl_nds, &ctrl_bl_head, 0);		
										
										put_el_db_num(&live_fd, &live_fd_idx, &live_fd_limit, &live_fd_idx_limit, &live_idx , event_file_fd);;
																			
								
										cur_event_info_tmp = get_head(ctrl_nds);		
										clear_name(out_buf_console, j_cnsl);
										cur_event_info_tmp->name_file_size = strlen(out_buf_console);
					
										strncpy(cur_event_info_tmp->file_name, out_buf_console, cur_event_info_tmp->name_file_size);										
										cur_event_info_tmp->file_size = (size_t)fl_size;
										cur_event_info_tmp->num_data = (uint32_t)fl_size;
										cur_event_info_tmp->left_file_size = cur_event_info_tmp->file_size;
										cur_event_info_tmp->rem_file = cur_event_info_tmp->left_file_size % CHUNK_SIZE;
										cur_event_info_tmp->file_wr_fd = cur_event_info->file_wr_fd;
										
										put_el_db_num(&live_fd, &live_fd_idx, &live_fd_limit, &live_fd_idx_limit, &live_idx , cur_event_info_tmp->file_wr_fd);
										

										write_client_msg(conn_fd, tmp_command_file_send, tmp_j_cnsl, 1, 0, 's');									
																		
										break;
									}
			
								}
								
							} else {
														
								if ('f' == command && 1 == s_com ){
									strncpy(tmp_command_file_send, out_buf_console, 100);								
									tmp_j_cnsl = j_cnsl;
								
									char wr_fl_r_p_msg[] = "[system]: Write file path or type /exit\n";
									write_common_console_msg(wr_fl_r_p_msg, &wbuf, &j_cnsl, 40, &fl_line_feed, &total_lines, out_buf_console);							
									fl_file_wr++;								
								}						
								
								else if (1 == fl_shell_src_init){ 
									write_client_msg(conn_fd, out_buf_console, j_cnsl, 3, 0, 's');	
								}
								
								else if (1 == fl_browse_active){								
									if (write(master_fd_loc, out_buf_console, j_cnsl) != j_cnsl)
										err(EXIT_FAILURE, "[system]: partial/failed write (master_fd_loc) ");						
								}
								
								else if ('b' == command &&  0 == fl_browse_active ){
										
										char slave_name_loc[MAX_SNAME] = {0};
										char *shell_loc = NULL;
									
										fl_browse_active++;
										child_pid_loc = -1;
										
										struct termios tty_orig = {0};

				
										if (tcgetattr(STDIN_FILENO, &tty_orig) == -1)
											err(EXIT_FAILURE, "[system]: tcgetattr ");
										
										struct winsize ws_loc = {0};
										if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws_loc) < 0)
											err(EXIT_FAILURE, "[system]: ioctl-TIOCGWINSZ ");

						
										child_pid_loc = pty_fork(&master_fd_loc, slave_name_loc, MAX_SNAME, &tty_orig, &ws_loc);
										if (child_pid_loc == -1)
											err(EXIT_FAILURE, "[system]: pty fork loc ");

					
										if (child_pid_loc == 0) {       
											if (-1 == close_all_live_fd(live_fd, live_fd_idx, live_fd_limit, &live_fd_idx_limit, &live_idx)){
												kill(getppid(), SIGTERM);									
												exit(1);
											}
											
											shell_loc = getenv("SHELL");						
											if (shell_loc == NULL || *shell_loc == '\0')
												shell_loc = "/bin/sh";

											execlp(shell_loc, shell_loc, (char *) NULL);
											err(EXIT_FAILURE, "[system]: execlp file browse loc.");      
										}		
																	
										set_non_block_fd(master_fd_loc);								
										add_event(epoll_fd, master_fd_loc, ctrl_nds, &ctrl_bl_head, 1);
										put_el_db_num(&live_fd, &live_fd_idx, &live_fd_limit, &live_fd_idx_limit, &live_idx , master_fd_loc);									
								}
								
								else {
									write_client_msg(conn_fd, out_buf_console, j_cnsl, 1, 0, 's');	
								}
							}	
							
							if (-1 == ioctl(STDIN_FILENO, TIOCGWINSZ, &wbuf))
								err(EXIT_FAILURE, "[system]: error 3ioctl winsize ");
								
								
							if ('c' == command) {
								write_n(STDOUT_FILENO, "\033[2J\033[H", 7);					
								write_n(STDOUT_FILENO, "\0337", 2);	
								total_lines = 0;
							
							
							
							} else {							
								if (total_lines){
									write_n(STDOUT_FILENO, "\r", 1);
									write_n(STDOUT_FILENO, "\033[K", 3);
															
									
									for (int i = 0; i < total_lines ; i++){
										write_n(STDOUT_FILENO, "\033M", 2);	
										write_n(STDOUT_FILENO, "\033[K", 3);	
									
									}
								} else {
									write_n(STDOUT_FILENO, "\r>:", 3);	
									write_n(STDOUT_FILENO, "\033[K", 3);														
								}
							}
							
							printf("\033[%d;%dH", wbuf.ws_row, 0);
							write_n(STDOUT_FILENO, "\r>:", 3);	
							
							memset(out_buf_console, 0, j_cnsl);
							j_cnsl = 0;	 
							
							continue;
						}	
					}
					
					else {
						static char limit_msg[] = "[system]: Message has exceed the limit length.\n"; 
						write_common_console_msg(limit_msg, &wbuf, &j_cnsl, 47, &fl_line_feed, &total_lines, out_buf_console);	
					}	
					
				
					if (0 == ((j_cnsl+3) % wbuf.ws_col)){
						total_lines++;			
					}
				

					out_buf_console[j_cnsl++] = el;					
					write_n(STDOUT_FILENO, in_buf_console, 1);	
				}	
			}
		
		
			
			else if (event_file_fd == cur_event_info->fd){

				while (1){
					if (-1 == (s = read(event_file_fd, &u, UINT64_T_SZ))){
						if (EAGAIN == errno || EWOULDBLOCK == errno)
							break;
						else if (EINTR == errno) 
							continue;
						err(EXIT_FAILURE, "[system]: read event file fd ");	
					}
					break;
				}

			
				if (0 == cur_event_info->fl_file_name){
					write_client_msg(conn_fd,  cur_event_info->file_name, cur_event_info->name_file_size, 2, 0, 's');
					write_to_event_fd(&u, event_file_fd);	
					cur_event_info->fl_file_name++;								
				}
				
				else if (0 == cur_event_info->fl_file_size){
					write_client_msg(conn_fd,  &cur_event_info->num_data, 0, 2, 0, 'n');
					write_to_event_fd(&u, event_file_fd);
					cur_event_info->fl_file_size++;									
				}
			
				else {
					size_t too_send = 0;
				
					if (cur_event_info->file_size <= CHUNK_SIZE){			
						while (1){
							if (-1 == (too_send = read(cur_event_info->file_wr_fd, cur_event_info->file_chunk, cur_event_info->file_size))){
								if (EINTR == errno) 
									continue;
								err(EXIT_FAILURE, "[system]: read file_wr_fd error ");	
							}
							break;
						}
						
						write_client_msg(conn_fd,  cur_event_info->file_chunk, too_send, 2, 0, 's');
					 
						del_el_db_num(live_fd, live_fd_idx, live_fd_limit, &live_fd_idx_limit, &live_idx, event_file_fd);	
						if (-1 == close(event_file_fd))
							err(EXIT_FAILURE, "[system]: event_file_fd error close ");										
						event_file_fd = -1;
												
						del_el_db_num(live_fd, live_fd_idx, live_fd_limit, &live_fd_idx_limit, &live_idx, cur_event_info->file_wr_fd);		
						clear_file_send(cur_event_info, 's', 'c');									
					}
					
					else{	
						if (cur_event_info->left_file_size > 0){
							while (cur_event_info->left_file_size > 0){

								if (cur_event_info->rem_file > 0) {
									while (1){
										if (-1 == (too_send = read(cur_event_info->file_wr_fd, cur_event_info->file_chunk, cur_event_info->rem_file))){
											if (EINTR == errno) 
												continue;
											err(EXIT_FAILURE, "[system]: read rem chunk error ");	
										}
										break;
									}		
									
									write_client_msg(conn_fd,  cur_event_info->file_chunk, cur_event_info->rem_file, 2, 0, 's');

									cur_event_info->left_file_size -= cur_event_info->rem_file;
									cur_event_info->offset_file += cur_event_info->rem_file;
									cur_event_info->rem_file = 0;
									for (int i = 0; i < cur_event_info->rem_file; cur_event_info->file_chunk[i++] = 0);	
																					
									if (cur_event_info->left_file_size > 0){
										write_to_event_fd(&u, event_file_fd);
										break;
									}
								} 
								
								else {								
									while (1){
										if (-1 == (too_send = read(cur_event_info->file_wr_fd, cur_event_info->file_chunk, MAX_LINE))){
											if (EINTR == errno) 
												continue;
											err(EXIT_FAILURE, "[system]: chunk error ");	
										}
										break;
									}	
			
									write_client_msg(conn_fd,  cur_event_info->file_chunk, too_send, 2, 0, 's');
								
									cur_event_info->left_file_size -= too_send;
									cur_event_info->offset_file += too_send;
									for (int i = 0; i < too_send; cur_event_info->file_chunk[i++] = 0);
									
									if (cur_event_info->left_file_size > 0){
										write_to_event_fd(&u, event_file_fd);
										break;
									}	
								}				
							}
						}
						
						else {
							del_el_db_num(live_fd, live_fd_idx, live_fd_limit, &live_fd_idx_limit, &live_idx, event_file_fd);	
							if (-1 == close(event_file_fd))
								err(EXIT_FAILURE, "[system]: event_file_fd error close ");									
							event_file_fd = -1;	
							
							del_el_db_num(live_fd, live_fd_idx, live_fd_limit, &live_fd_idx_limit, &live_idx, cur_event_info->file_wr_fd);
							clear_file_send(cur_event_info, 's', 'c');							
						}
					}
				}
			}
			
			else if (master_fd_loc == cur_event_info->fd){	
				for (;;){
					if (-1 == (n_read = read(master_fd_loc, line , MAX_LINE))){
						if (EAGAIN == errno || EWOULDBLOCK == errno){ 
							break;
						}
						else if (EINTR == errno){
							continue;	
						}
						else if (EIO == errno){
							close(master_fd_loc);
							static char eio_master_loc_msg[] = "[system]: EIO master loc error.\n";
							write_common_console_msg(eio_master_loc_msg, &wbuf, &j_cnsl, 32, &fl_line_feed, &total_lines, out_buf_console);
							break;
						}
						else err(EXIT_FAILURE, "[system]: read client master loc error ");						
					}			
				
					if  (0 == n_read){ 
						break;
					}
					
					write_common_console_msg(line, &wbuf, &j_cnsl, n_read, &fl_line_feed, &total_lines, out_buf_console);
					memset(line, 0, n_read);		
				}
			}
						
			else if (master_fd == cur_event_info->fd){					
				for (;;){
					if (-1 == (n_read = read(master_fd, line, MAX_LINE))){
						if (EAGAIN == errno || EWOULDBLOCK == errno){ 
							break;
						}							
						else if (EINTR == errno){
							continue;	
						}
						else if (EIO == errno){
							close(master_fd);
							static char eio_master_msg[] = "[system]: EIO master error.\n";
							write_common_console_msg(eio_master_msg, &wbuf, &j_cnsl, 29, &fl_line_feed, &total_lines, out_buf_console);													
							break;
						}
						else err(EXIT_FAILURE, "[system]: read client master error ");						
					}		
					
					if  (0 == n_read){ 
						break;
					}
					
					write_common_console_msg(line, &wbuf, &j_cnsl, n_read, &fl_line_feed, &total_lines, out_buf_console);
					write_client_msg(conn_fd, line, n_read, 3, 0, 's');
					memset(line, 0, n_read);
				}
			} 
			
			else if (total_events[n].events & EPOLLIN | EPOLLET){
			
				int result = 0;
				
				for (;;){	
					char buf[CHUNK_SIZE] = {0};	
					
					if (-1 == result){
						break;
					}
					
					if (-1 == (n_read = read(cur_event_info->fd, buf, CHUNK_SIZE))){
						if (EAGAIN == errno || EWOULDBLOCK == errno){ 
							break;
						}
						else if (EINTR == errno){
							continue;
						}
						else if (ECONNRESET == errno){
							exit(1);
						}
						else {
							err(EXIT_FAILURE, "[system]: read client error, ");
						}
					}
						
					if  (0 == n_read){  
						exit(0);
					}
					
					ssize_t cur_read = 0;
					
				
					while ((n_read - cur_read) > 0){
						result = read_incoming_data(&n_read, &cur_read,  buf, cur_event_info);	

						
						if (!result){
							continue;
						}	
						if (-1 == result){
							clear_msg_buf(cur_event_info);
							break;
						}
						if (1 == result){
		
							if (1 == cur_event_info->type_msg){  		  	// normal messages:
								if (1 == cur_event_info->notif_rcv){ 	  	// server confirm file sending src
									cur_event_info->fl_file_src_init = 1;					
									write_to_event_fd(&u, event_file_fd);											
								}
								
								else if (2 == cur_event_info->notif_rcv){   // server confirm file sending dst
									cur_event_info->fl_file_dst_init = 1;				
								}
							 
								else if (3 == cur_event_info->notif_rcv){	// server confirm shell managing src							
									fl_shell_src_init = 1; 							
								}
								
								else if (4 == cur_event_info->notif_rcv){   // server confirm shell managing dst
								
									char *shell = NULL;
								    char slave_name[MAX_SNAME] = {0};
									
									child_pid = 0;
									fl_shell_dst_init = 1;
							
									struct termios tty_orig = {0};
									
									if (-1 == tcgetattr(STDIN_FILENO, &tty_orig))
										err(EXIT_FAILURE, "[system]: shell managing tcgetattr ");
									
									struct winsize ws = {0};
									if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0)
										err(EXIT_FAILURE, "[system]: shell managing ioctl-TIOCGWINSZ ");

					
									child_pid = pty_fork(&master_fd, slave_name, MAX_SNAME, &tty_orig, &ws);
									if (-1 == child_pid)
										err(EXIT_FAILURE, "[system]: shell managing pty_fork ");

								
									if (0 == child_pid) { 
										if (-1 == close_all_live_fd(live_fd, live_fd_idx, live_fd_limit, &live_fd_idx_limit, &live_idx)){
												kill(getppid(), SIGTERM);									
												exit(1);
										}								
															
										shell = getenv("SHELL");
										if (shell == NULL || *shell == '\0')
											shell = "/bin/sh";

										execlp(shell, shell, (char *) NULL);
										err(EXIT_FAILURE, "[system]: execlp");      
									}
									
									
									set_tty_settings(NULL);
									set_non_block_fd(master_fd);
									add_event(epoll_fd, master_fd, ctrl_nds, &ctrl_bl_head, 0);	
									
									put_el_db_num(&live_fd, &live_fd_idx, &live_fd_limit, &live_fd_idx_limit, &live_idx , master_fd);

								}								
								
								// standart message
								write_common_console_msg(cur_event_info->msg_chat_buf, &wbuf, &j_cnsl, cur_event_info->msg_chat_idx, &fl_line_feed, &total_lines, out_buf_console);
									
																			
							}
							
							else if (2 == cur_event_info->type_msg){	// file sending
							
								if (1 == cur_event_info->fl_file_dst_init){										
									if (0 == cur_event_info->fl_file_name){
										cur_event_info->fl_file_name = 1;									
										strncpy(cur_event_info->file_name, cur_event_info->msg_chat_buf, MAX_LINE);		
									}
									
									else if (0 == cur_event_info->fl_file_size) {
										cur_event_info->fl_file_size = 1;	
										cur_event_info->left_file_size = cur_event_info->num_data;
																				
										static char edited_file_name[MAX_LINE];
										snprintf(edited_file_name, MAX_LINE, "[system] %s", cur_event_info->file_name);
										
										while (1) {
											if (-1 == (cur_event_info->file_read_fd = open(edited_file_name, O_WRONLY | O_CREAT | O_TRUNC, 0644))){		
												if (EINTR == errno){
													continue;
												}
												else if (EACCES == errno){
													static char filew_err_acc_msg[] = "[system]: You don't have acces of the file.\n";
													write_common_console_msg(filew_err_acc_msg, &wbuf, &j_cnsl, 44, &fl_line_feed, &total_lines, out_buf_console);																			
													break;
												}
												else if (ENOTDIR == errno){
													static char filew_err_dir_msg[] = "[system]: The file is directory.\n";
													write_common_console_msg(filew_err_dir_msg, &wbuf, &j_cnsl, 33, &fl_line_feed, &total_lines, out_buf_console);												
													break;
												}
												else if (ENOENT == errno){
													static char filew_err_eno_msg[] = "[system]: The file doesn't exists.\n";
													write_common_console_msg(filew_err_eno_msg, &wbuf, &j_cnsl, 35, &fl_line_feed, &total_lines, out_buf_console);																				
													break;
												}
												err(EXIT_FAILURE, "[system]: Error user file.");
											}
											break;
										}
										for (int i = 0; i < MAX_LINE; edited_file_name[i++] = 0);
										put_el_db_num(&live_fd, &live_fd_idx, &live_fd_limit, &live_fd_idx_limit, &live_idx , cur_event_info->file_read_fd);											
									}	
									
									else if (0 == cur_event_info->fl_file_chunks){
										if (cur_event_info->left_file_size > 0){												
											if (-1 == write(cur_event_info->file_read_fd, cur_event_info->msg_chat_buf, cur_event_info->msg_chat_idx))
												err(EXIT_FAILURE, "[system]: write file error");
										
											cur_event_info->file_offset += cur_event_info->msg_chat_idx;
											cur_event_info->left_file_size -= cur_event_info->msg_chat_idx;
															
											if (cur_event_info->left_file_size == 0){
												del_el_db_num(live_fd, live_fd_idx, live_fd_limit, &live_fd_idx_limit, &live_idx, cur_event_info->file_read_fd);
												clear_file_send(cur_event_info, 'd', 'c');
												
												write_common_console_msg("[system]: File received.\n", &wbuf, &j_cnsl, strlen("[system]: File received.\n"), &fl_line_feed, &total_lines, out_buf_console);								
											}	
										}
									}		
								}						
							}
							
							else if (3 == cur_event_info->type_msg){ 					
								if (1 == cur_event_info->notif_rcv){ // cancel src
									fl_shell_src_init = 0;							
									write_common_console_msg(cur_event_info->msg_chat_buf, &wbuf, &j_cnsl, cur_event_info->msg_chat_idx, &fl_line_feed, &total_lines, out_buf_console);
												
								}
								
								else {	// write to src							
									if (1 == fl_shell_src_init){				
										write_common_console_msg(cur_event_info->msg_chat_buf, &wbuf, &j_cnsl, cur_event_info->msg_chat_idx, &fl_line_feed, &total_lines, out_buf_console);													
									} 
									
									else{ // write to dst	
										if (-1 != master_fd){
											if (write(master_fd, cur_event_info->msg_chat_buf, cur_event_info->msg_chat_idx) != cur_event_info->msg_chat_idx)
												err(EXIT_FAILURE, "[system]: partial/failed write (master_fd)");	
											if (2 == cur_event_info->notif_rcv){
												fl_shell_dst_init = 0; // cancel dst
												if (-1 == close(master_fd))
													err(EXIT_FAILURE, "[system] error closeing master_fd.\n");
												del_el_db_num(live_fd, live_fd_idx, live_fd_limit, &live_fd_idx_limit, &live_idx, master_fd);
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

    fprintf(stderr, "[system]: Client shutdown.\n");

    exit(0);
}


