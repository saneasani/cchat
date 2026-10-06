#include "events.h"



control_nodes *create_cn(){
    control_nodes *ctrl_nds = NULL;
	if (NULL == (ctrl_nds = ecalloc(1, sizeof(control_nodes))))
		err(EXIT_FAILURE, "[system]: create_cn, ");
	
	ctrl_nds->head = NULL;
	ctrl_nds->free_node_head = NULL;
    ctrl_nds->allocated_node = NULL;
    ctrl_nds->size_left = 0;

	return ctrl_nds;
}



control_blocks *create_bl(){
    control_blocks *cntrl_bl = NULL;
	if (NULL == (cntrl_bl = ecalloc(1, sizeof(control_blocks))))
		err(EXIT_FAILURE, "[system]: create_bl ");
	
	cntrl_bl->alloc_block = NULL;
	cntrl_bl->next = NULL;

	return cntrl_bl;
}



event_info *get_new_node(control_nodes *cn, control_blocks **cb_head){
	event_info *tmp = NULL;
	
	if (NULL != cn->free_node_head){
		tmp = cn->free_node_head;
		cn->free_node_head = cn->free_node_head->next;
		if (NULL != cn->free_node_head)
			cn->free_node_head->prev = NULL;

	} else {
		if (NULL == cn->allocated_node || 0 == cn->size_left){
			cn->allocated_node = ecalloc(NODE_QUANTITY, sizeof(event_info));
			if (NULL == cn->allocated_node)
				err(EXIT_FAILURE, "[system]: calloc cn->allocated_node ");		
			cn->size_left = NODE_QUANTITY;
				
			control_blocks *cb_new = create_bl();
			cb_new->alloc_block = cn->allocated_node;
			cb_new->next = *cb_head;
			*cb_head = cb_new;
		}
		tmp = cn->allocated_node++;
		cn->size_left--;
	}
	return tmp;
}



event_info *push_node(control_nodes *cn, int fd, control_blocks **cb){
	if (NULL == cn){
		fprintf(stderr, "[system]: push_node, cn is NULL.\n");
		return NULL;
	}
	if (NULL == cb){
		fprintf(stderr, "[system]: push_node, cb is NULL.\n");
	}		
	
	if (fd < 0){
		fprintf(stderr, "[system]: push_node, fd < 0.\n");
		return NULL;
	}
	
	event_info *new_node = NULL;
	if (NULL == (new_node = get_new_node(cn, cb)))
		return NULL;
	
	
	if (NULL == cn->head){
		cn->head = new_node;
		new_node->next = NULL;
		new_node->prev = NULL;
	
	} else {
		new_node->next = cn->head;
		cn->head = new_node;
		cn->head->next->prev = new_node;
		cn->head = new_node;
		cn->head->prev = NULL;
	}
		
	new_node->fd = fd;
	for (int i = 0; i < 10; i++)
		new_node->name[i] = '\0';
	
	new_node->init_name = 0;
	new_node->entry_idx = 0;
	new_node->global = 1;
	
	new_node->fl_notif_rcv = 0;
	new_node->notif_rcv = 0;
	new_node->type_msg = 0;
	new_node->start_msg = 0;
	new_node->flag_len_msg = 0;
	new_node->msg_chat_idx = 0;
	
	for (int i = 0; i < MAX_LINE; i++)
		new_node->msg_chat_buf[i] = '\0';
	new_node->len_chat_msg = 0;
	for (int i = 0; i < 2; i++)
		new_node->len_chat_msg_arr[i] = '\0';
		
	
	// --- private ---
	new_node->private = NULL;
	new_node->init_pr = 0;

	// --- shell ---
	new_node->init_src_shell = 0;
	new_node->init_dst_shell = 0;
	
	new_node->shell = NULL;
	
	// --- file ---	
	new_node->fl_file_src_init = 0;
	new_node->fl_file_dst_init = 0;
	new_node->fl_file_name = 0;
	new_node->fl_file_size = 0;
	new_node->fl_file_chunks = 0;	
	new_node->file_read_send_fd = 0;
	new_node->file_offset = 0;
	new_node->msg_file_idx = 0;
	new_node->msg_num_idx = 0;
	new_node->file_wr_fd = -1;
	new_node->file_read_fd = -1;

	
	
	new_node->reserved = 0;
	new_node->flag_reserved_msg = 0;
	for (int i = 0; i < 2; i++)
		new_node->len_reserved_arr[i] = '\0';

	for (int i = 0; i < MAX_LINE; new_node->file_name[i++] = '\0');
	new_node->name_file_size = 0;
	new_node->file_size = 0;
	new_node->left_file_size = 0;
	new_node->offset_file = 0;
	new_node->rem_file = 0;
	for (int i = 0; i < MAX_LINE; new_node->file_chunk[i++] = '\0');
	
	// --- timer ---
	//new_node->ts = {0};
	for (int i = 0; i < 2; i++)
		new_node->timer_rq_db_p[i] = -1;
	for (int i = 0; i < 2; i++)
		new_node->timer_rq_db_f[i] = -1;	
	for (int i = 0; i < 2; i++)
		new_node->timer_rq_db_s[i] = -1;
	
	new_node->cur_rq = 0;
	new_node->limit_rq = 10;
	
	
	return new_node;
}

void return_node(control_nodes *cn, event_info *node){
	if (NULL == cn){
		fprintf(stderr, "[system]: return_node, cn is NULL.\n");
		return;	
	}	
	if (NULL == node){
		fprintf(stderr, "[system]: return_node, node is NULL.\n");
		return;	
	}	
	
	node->fd = -1;
	for (int i = 0; i < 10; i++)
		node->name[i] = '\0';
	
	node->init_name = 0;
	node->entry_idx = 0;
	node->global = 1;
	
	node->fl_notif_rcv = 0;
	node->notif_rcv = 0;
	node->type_msg = 0;
	node->start_msg = 0;
	node->flag_len_msg = 0;
	node->msg_chat_idx = 0;
	
	for (int i = 0; i < MAX_LINE; i++)
		node->msg_chat_buf[i] = '\0';
	node->len_chat_msg = 0;
	for (int i = 0; i < 2; i++)
		node->len_chat_msg_arr[i] = '\0';
		
	
	// --- private ---
	node->private = NULL;
	node->init_pr = 0;

	// --- shell ---
	node->init_src_shell = 0;
	node->init_dst_shell = 0;
	
	node->shell = NULL;
	
	// --- file ---
	node->fl_file_src_init = 0;
	node->fl_file_dst_init = 0;
	node->fl_file_name = 0;
	node->fl_file_size = 0;
	node->fl_file_chunks = 0;	
	node->file_read_send_fd = -1;
	node->file_offset = 0;
	node->msg_file_idx = 0;
	node->msg_num_idx = 0;
	node->file_wr_fd = -1;
	node->file_read_fd = -1;
	
	
	node->reserved = 0;
	node->flag_reserved_msg = 0;
	for (int i = 0; i < 2; i++)
		node->len_reserved_arr[i] = '\0';

	for (int i = 0; i < MAX_LINE; node->file_name[i++] = '\0');
	node->name_file_size = 0;
	node->file_size = 0;
	node->left_file_size = 0;
	node->offset_file = 0;
	node->rem_file = 0;
	for (int i = 0; i < MAX_LINE; node->file_chunk[i++] = '\0');
	
	// --- timer ---
	for (int i = 0; i < 2; i++)
		node->timer_rq_db_p[i] = -1;
	for (int i = 0; i < 2; i++)
		node->timer_rq_db_f[i] = -1;	
	for (int i = 0; i < 2; i++)
		node->timer_rq_db_s[i] = -1;
	
	node->cur_rq = 0;
	node->limit_rq = 10;
}

void pop_node(control_nodes *cn, event_info *node){
	if (NULL == cn){
		fprintf(stderr, "[system]: pop_node, cn is NULL.\n");
		exit(1);
	}	
	if (NULL == node){
		fprintf(stderr, "[system]: pop_node, node is NULL.\n");
		exit(1);	
	}	

	event_info *tmp = NULL;
	
	if (node == cn->head){
		if (NULL == node->next){
			tmp = node;
			cn->head = NULL;
		
		} else {
			tmp = node;
			node->next->prev = NULL;
			cn->head = cn->head->next;
		}
	} else {
		if (NULL == node->next){
			tmp = node;
			node->prev->next = node->next;
		
		} else {
			tmp = node;
			node->next->prev = node->prev;
			node->prev->next = node->next;
		}
	}
	
	return_node(cn, tmp);
}



event_info *get_head(control_nodes *cn){
        return cn->head;
}




void clear_blocks(control_blocks **cb_head){
	if (NULL == cb_head){
		fprintf(stderr, "[system]: clear_blocks, cb_head is NULL.\n");
		exit(1);
	}
	
	control_blocks *itr_bl = *cb_head;
	control_blocks *prev_bl = NULL;
	
	int count = 1;

	while(itr_bl){
		prev_bl = itr_bl;
		itr_bl = itr_bl->next;
	
		free(prev_bl->alloc_block);
		free(prev_bl);
	}
	*cb_head = NULL;
}


 int create_epoll(){
	int epoll_fd = -1;
	if (-1 == (epoll_fd = epoll_create1(0)))
			err(EXIT_FAILURE, "[system]: epoll create error ");

	return epoll_fd;
}


int add_event(int epoll_fd, int fd, control_nodes *cn, control_blocks **cb, int action){
	if (epoll_fd < 0){
		fprintf(stderr, "[system]: add_event, epoll_fd < 0.\n");
		return -1;
	}
	if (fd < 0){
		fprintf(stderr, "[system]: add_event, fd < 0.\n");
		return -1;
	}
	if (action < 0 || action > 1){
		fprintf(stderr, "[system]: add_event, unsupported action.\n");
		return -1;
	}

	struct epoll_event ev = {0};
	if (0 == action)
		ev.events = EPOLLIN;
	else if (1 == action)
		ev.events = EPOLLIN | EPOLLET;

	ev.data.ptr = push_node(cn, fd, cb);
	if (NULL == ev.data.ptr){
		fprintf(stderr, "[system]: add_event, event node is NULL.\n");
		return -1;
	}

	if (-1 == epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev))
		err(EXIT_FAILURE, "[system]: add_event, epoll_ctl error ");
		
	return 1;
}
