#define _GNU_SOURCE

#include <dlfcn.h>
#include <string.h>
#include <stdio.h> 
#include <unistd.h>

#include "find_dyn_addr.h"

void *find_dyn_addr(const char* symbol)
{
	void *f = dlsym(RTLD_NEXT, symbol); 
	const char *err = dlerror();
	if(err != NULL)
	{
		size_t len = strnlen(err, 0xFF);  
		write(STDERR_FILENO, err, len); 
		return NULL; 	
	}
	else
	{
		char buf[64];
		size_t len = snprintf(buf, 64, "The process used %s\n", symbol); 
		write(STDERR_FILENO, buf, len); 
		return f; 
	} 
}
