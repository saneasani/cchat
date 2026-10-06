#include "tools.h"


long proceed_strtol(const char *port){
	if ('0' == port[0] && '\0' != port[1]){
	   fprintf(stderr, "[system]: proceed_strtol, If port number != 0 then can't start from 0\n");
	   exit(EXIT_FAILURE);
	}	
	errno = 0;
	char *end_ptr = NULL;
	long port_num = strtol(port, &end_ptr, 10);
	
	if (ERANGE == errno)
		err(EXIT_FAILURE, "[system]: proceed_strtol, strtol error ");
		
	if ('\0' != *end_ptr || end_ptr == port){
	   fprintf(stderr, "[system]: proceed_strtol, Should be only numbers\n");
	   exit(EXIT_FAILURE);
	}
	if (0 > port_num){
		fprintf(stderr, "[system]: proceed_strtol, Negative port number is not available.\n");
		exit(EXIT_FAILURE);
	}		
	if (65535 < port_num){
		fprintf(stderr, "[system]: proceed_strtol, Maximum port is 65535.\n");
		exit(EXIT_FAILURE);
	}
	
	return port_num;
}

void clear_name(char *name, size_t sz){
	if (NULL == name){
		fprintf(stderr, "[system]: clear_name, name is NULL.\n");
		exit(1);
	}
	if (sz < 0){
		fprintf(stderr, "[system]: clear_name, sz < 0.\n");
		exit(1);
	}
 
   char *ptr = name, *base = name;
   size_t idx_found = 0;
   
   for (;*ptr; ptr++){
       if (*ptr == '/'){
           idx_found = ptr - base;  
       }
   }

   if (idx_found){
	   ptr = name + idx_found + 1;
	   size_t limit = sz - idx_found;
	   
	   for (int i = 0; i < sz; i++) {
	       if (i < limit){
			  *base = *ptr;    
	       } else {
	          *base = '\0';
	       }
			
		base++;
		ptr++;
	   }
   }  
}


int parse_name(char buf[], size_t len){
	if (NULL == buf){
		fprintf(stderr, "[system]: parse_name, buf is NULL.\n");
		return -1;
	}
	if (len < 5 || len > 11) {
		fprintf(stderr, "[system]: parse_name, len < 5 or > 11");
		return -1;
	}
	
	size_t last_idx = len;
	size_t pre_last_idx = len-1; 

	for (int i = 0; i < len && buf[i]; i++){
		if (!(buf[i] >= 48 && buf[i] <= 57 ||
			  buf[i] >= 65 && buf[i] <= 90 ||
			  buf[i] >= 97 && buf[i] <= 122)
			){
			if (i == last_idx && buf[i] == '\0') continue;
			else if (i == pre_last_idx && buf[i] == '\n') 
				buf[i] = '\0';
			else return -1;			
		}
	}
	return 1;
}

size_t get_size_num(size_t num){
	if (num < 0){
		fprintf(stderr, "[system]: get_size_num, num < 0.\n");
		exit(1);
	}
	
    size_t count = 1;
    
    while(num/10){
        num/=10;
        count++;
    }
    return count;
}


void *ecalloc(size_t n, size_t el_size){
	void *rv = NULL;
	if (NULL == (rv = calloc(n, el_size))) 
		err(EXIT_FAILURE, "[system]: calloc() failed "); 
	
	return rv;
}

void *erealloc(void *p, size_t n){
	void *rv = NULL; 
	if (NULL == (rv = realloc(p, n))) 
		err(EXIT_FAILURE, "[system]; realloc() failed ");
    
	return rv;
}

  
  
