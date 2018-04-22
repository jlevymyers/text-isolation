#define _GNU_SOURCE

#include <dlfcn.h>
#include <string.h>
#include <stdio.h> 
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "find_dyn_addr.h"
#include "proc_maps_parser/pmparser.h"
#include "proc_maps_parser/pmparser.c"

#include <stdint.h>

const char* const DLWRAP_PATH = "/gpfs/main/home/jlevymye/course/cs2951/text-isolation/wrapper.so";
const char* const DL_PATH  = "/lib/x86-64-linux-gnu/libdl-2.24.so";
const char* const VSYSCALL ="[vsyscall]";

extern uintptr_t __base_addr; 

#define IN_RANGE(a, b, c) (a <= c) && (c < b)

//state variable to prevent hook from getting hooked
int call_count = 0;

int on = 0;

procmaps_struct* exec_map = NULL;

const char * ALWAYS_EXEC[3] =  {"/gpfs/main/home/jlevymye/course/cs2951/text-isolation/wrapper.so",
	"/lib/x86-64-linux-gnu/libdl-2.24.so",
	"[vsyscall]"};

/*
 * iterates through the map of all memory regions
 * and saves the executable region
 */ 

int save_exec_regions(procmaps_struct* map){
	procmaps_struct* iter = map;
       	int count = 0;
	while(iter != NULL){
		if(iter -> is_x){
			count++;
		}
		iter = iter -> next; 
	}
	exec_map = (procmaps_struct*) malloc(count*sizeof(procmaps_struct));
	if(exec_map == NULL){
		return -1; 
	}
	int i = 0;
	iter = map; 	
	while(iter != NULL){
		if(iter -> is_x){
			memcpy(&exec_map[i], iter, sizeof(procmaps_struct));
			i++; 
		}
		iter = iter -> next; 
	}
	return count;
}

/*
 * constructor for runtime system that executes before the program
 * creates an initial mapping of the text regions, and marks it read only
 */ 

__attribute__((constructor))
void remap_code_ctor()
{
	printf("******************\n\n");	
	pid_t pid = getpid();
	procmaps_struct *maps = pmparser_parse(pid); 
	int num_regions = save_exec_regions(maps); 
	mprotect(exec_map, sizeof(procmaps_struct) * num_regions, PROT_READ);
	printf("Runtime constructor running...\n");
	printf("Initial Memory Mapping\n"); 
	for(int i = 0; i < num_regions; i++){
		printf("%d: %s 0x%lx-0x%lx\n", i, exec_map[i].pathname, (uintptr_t) exec_map[i].addr_start, (uintptr_t) exec_map[i].addr_end);	
	}
	printf("base address: 0x%lx\n", __base_addr);
	uintptr_t return_addr = (uintptr_t) __builtin_return_address(0); 
	printf("return address: 0x%lx\n", return_addr);
	printf("******************\n\n");
	on = 1;	
}

/*
 * destructor which runs after the target program exits 
 * frees the text map 
 */ 

__attribute__((destructor))
void remap_code_dtor()
{
	printf("Runtime destructor running\n");
	if(exec_map != NULL){
		free(exec_map);
	}
	on = 0;
}

/*
 * function which marks the code region containing symbol, executable
 *	marks all other executable code regions read only
 */


int remap_code(const char* symbol, uintptr_t fun)
{
	if(call_count == 0){
		call_count ++; 
		pid_t pid = getpid();
		procmaps_struct *maps = pmparser_parse(pid); 
		procmaps_struct *start = maps; 
		//pmparser_print(maps, -1); 	
		int err; 

		while(maps != NULL)
		{
			printf("%s\n", maps -> pathname); 
			//TODO restore original memory mapping
			uintptr_t mem_start =  (uintptr_t) maps -> addr_start;  
			uintptr_t mem_end = (uintptr_t) maps -> addr_end;  
			uintptr_t ufun = (uintptr_t) fun;  
	
			//wrapper code - KEEP EXEC!
			if(!strncmp(DLWRAP_PATH, maps -> pathname, 600))
			{
				printf("wrapper.so 0x%lx-0x%lx, %ld\n", mem_start, mem_end, sizeof(DLWRAP_PATH)); 
			}
			else if(!strncmp(DL_PATH, maps -> pathname, sizeof(DL_PATH + 1)))
			{
				printf("libdl.so 0x%lx-0x%lx\n", mem_start, mem_end); 

			}
			else if(!strncmp(VSYSCALL, maps -> pathname, sizeof(VSYSCALL + 1)))
			{
				printf("vsyscall 0x%lx-0x%lx\n", mem_start, mem_end); 

			}

			//called function in memory region -- mark exec 
			else if(IN_RANGE(mem_start, mem_end, ufun))
			{
				//check if already marked EXEC
				if(maps -> is_x)
				{
					printf("CODE: 0x%lx-0x%lx ALREADY marked EXEC, contains SYMBOL %s:0x%lx\n", mem_start, mem_end, symbol, ufun); 
				}
				else
				{
					printf("CODE: 0x%lx-0x%lx marked NON-EXEC, contains SYMBOL %s:0x%lx\n", mem_start, mem_end, symbol, ufun); 
					//err = mprotect(maps->addr_start, maps->length, PROT_READ | PROT_EXEC);
					if(err)
					{
						printf("ERROR: mprotect PROT_READ | PROT_EXEC\n"); 
					}
				}
			}
			//check if memory region is executable -- mark read only
			else if(maps->is_x)
			{

				//mark previously executable region as read only
				printf("CODE: 0x%lx-0x%lx marked READ only\n", mem_start, mem_end); 
				//err = mprotect(maps -> addr_start, maps -> length, PROT_READ);
				if(err)
					printf("ERROR: mprotect PROT_READ\n");
			}
			//data region -- do nothing 
			else
			{
				printf("DATA region 0x%lx-0x%lx\n", mem_start, mem_end); 
			}
			maps = pmparser_next();
		}
		pmparser_free(start); 
		call_count--; 
	}
	return 0; 
}


/*
 * hook function for dynamic libraries, calls dynamic linker to resolve symbol
 * 	and then calls a function to handle the new memory mapping
 */

void *find_dyn_addr(const char* symbol)
{
	uintptr_t return_addr = (uintptr_t) __builtin_return_address(1); 
	char buf[256];
       	if(on){
		int len = snprintf(buf, 256, "Symbol: %s, return address: 0x%lx\n", symbol, return_addr);
		write(1, buf, len);
	}
	void *f = dlsym(RTLD_NEXT, symbol); 
	const char *err = dlerror();
	if(err != NULL)
	{
		size_t err_len = strnlen(err, 0xFF);  
		write(STDERR_FILENO, err, err_len); 
		return NULL; 	
	}
	else
	{
		//remap_code(symbol, f);
		return f; 
	}
}

