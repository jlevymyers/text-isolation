#define _GNU_SOURCE

#include "find_dym_addr.h"

#include<fcntl.h>

typedef int (*orig_open_f_type)(const char *pathname, int flags);


int open(const char *pathname, int flags, ...) 
{
	orig_open_f_type orig_open;
	orig_open = (orig_open_f_type) find_dym_addr("open");
	return orig_open(pathname, flags); 
}
