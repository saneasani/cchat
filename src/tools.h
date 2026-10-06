#ifndef H_TOOLS
#define H_TOOLS

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

size_t get_size_num(size_t num);
long proceed_strtol(const char *port);
void clear_name(char *name, size_t sz);
int parse_name(char buf[], size_t len);
void *ecalloc(size_t n, size_t el_size);
void *erealloc(void *p, size_t n);


#endif