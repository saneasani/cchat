 
#include "sys_server_msgs.h"
 
#include <err.h>
#include <stdio.h>
#include <string.h>

#include <stdlib.h>





char *get_info_server_system_msg(size_t idx, size_t *len){
	if (idx > limit_msgs){
		fprintf(stderr, "[system] get_info_server_system_msg, idx(%ld) > limit(%ld).\n", idx, len);
		exit(1);
	}
	if (idx < 0){
		fprintf(stderr, "[system] get_info_server_system_msg, idx(%ld) < 0.\n", idx, len);
		exit(1);
	}
		
	*len = all_msgs[idx].len;
	 
	return all_msgs[idx].msg;
}





char *get_server_system_msg(size_t idx, char *name, size_t ln, int num, size_t *ret_ln){
	size_t len = 0;
	int result = -1;
 
	char *tmp_ptr = NULL;
	static char ret_msg[100];
	for (int i = 0; i < 100; ret_msg[i++] = 0);
	
	tmp_ptr = get_info_server_system_msg(idx, &len);
	
	printf("tmp_ptr=>:%s\n", tmp_ptr);
	
	// sstm_msg = get_server_system_msg(6, cur_event_info->msg_chat_buf, cur_event_info->msg_chat_idx, -1, &sstm_msg_len);

    if (num >= 0 && NULL == name){  
        len += ln;
        if (len >= 100){
            fprintf(stderr, "[system]: get_server_system_ms, Unsupported length -> %zu", len);
            exit(1);
        }
        result = snprintf(ret_msg, 100, tmp_ptr, num); 
        if (-1 == result){
            err(EXIT_FAILURE, "[system]: get_server_system_ms, ");
        }
        
    	*ret_ln = (size_t)result; 
    	
        return ret_msg;
    }

	if (NULL == name && num < 0){
	    *ret_ln = len;
	    return get_info_server_system_msg(idx, &len);
	}
	
	else {
        len += ln;
        if (len >= 100){
			
            fprintf(stderr, "[system]: Unsupported length -> %zu", len);
            exit(1);
        }
		
		result = snprintf(ret_msg, 100, tmp_ptr, name); 
        if (-1 == result){
            err(EXIT_FAILURE, "[system]: get_server_system_ms, ");
        }
        
    	*ret_ln = (size_t)result;  

    	return ret_msg;
	}
}
											

