#define _GNU_SOURCE

#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <asm/unistd.h>

#include "printf/printf.h"
#include "printf/printf.c"

#include "proc_maps_parser/pmparser.h"
//#include "proc_maps_parser/pmparser.c"

#include "find_dyn_addr.h"


const char* const DLWRAP_PATH = "/gpfs/main/home/jlevymye/course/cs2951/text-isolation/wrapper.so";
const char* const DL_PATH  = "/lib/x86-64-linux-gnu/libdl-2.24.so";
const char* const VSYSCALL ="[vsyscall]";

extern uintptr_t __base_addr; 

#define IN_RANGE(a, b, c) (a <= c) && (c < b)

//state variable to prevent hook from getting hooked
int call_count = 0;

int on = 0;


//#define __NR_write 1
//#define __NR_mprotect 10

procmaps_struct* exec_map = NULL;

int num_exec_regions = 0;

char *main_name = "no main";

int ctor_done = 0;
int remap_code(uintptr_t fun, int all_exec);

const char * ALWAYS_EXEC[5] =  {"/gpfs/main/home/jlevymye/course/cs2951/text-isolation/wrapper.so",
	"/lib/x86_64-linux-gnu/libdl-2.24.so",
	"/lib/x86_64-linux-gnu/ld-2.24.so",
	//"/lib/x86_64-linux-gnu/libc-2.24.so",
	"[vdso]", 
	"[vsyscall]"};

#define ALWAYS_EXEC_SIZE 5

#define PRINT_SZ 1024 

int _write(int fd, const void *buf, size_t count){

	register int result asm("rax") = __NR_write;
	register int _fd asm("rdi") = fd; 
	register const void *_buf asm("rsi") = buf;
	register size_t _count asm("rdx") = count; 
	asm volatile(
			"syscall"
			: "+r" (result)
			: "r" (result), "r" (_fd), "r" (_buf), "r" (_count)
			: "memory");
	return result; 
}

int _mprotect(void *addr, size_t len, int prot){
	register int result asm("rax") = __NR_mprotect;
	register void *_addr asm("rdi") = addr; 
	register size_t _len asm("rsi") = len;
	register size_t _prot asm("rdx") = prot; 
	asm volatile(
			"syscall"
			: "+r" (result)
			: "r" (result), "r" (_addr), "r" (_len), "r" (_prot)
			: "memory");
	return result; 
}


void* _mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off){
	register void *result asm("rax") = (void *)__NR_mmap;
	register void *_addr asm("rdi") = addr; 
	register size_t _len asm("rsi") = len;
	register int _prot asm("rdx") = prot; 
	register int _flags asm("r10") = flags; 
	register int _fd asm("r8") = fd; 
	register int _off asm("r9") = off;
	asm volatile(
			"syscall"
			: "+r" (result)
			: "r" (result), "r" (_addr), "r" (_len), "r" (_prot), "r" (_flags), "r" (_fd), "r" (_off)
			: "memory");
	return result; 
}	


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
	exec_map = (procmaps_struct*) _mmap(NULL, count*sizeof(procmaps_struct), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(exec_map == NULL){
		return -1; 
	}
	int i = 0;
	iter = map; 	
	while(iter != NULL){
		if(iter -> is_x){
			exec_map[i].addr_start = iter -> addr_start;
			exec_map[i].addr_end = iter -> addr_end;
			exec_map[i].length = iter -> length;
			exec_map[i].is_r = iter -> is_r;
			exec_map[i].is_w = iter -> is_w;
			exec_map[i].is_x = iter -> is_x;
			exec_map[i].is_p = iter -> is_p;
			for(int j = 0; j <600; j++){
				exec_map[i].pathname[j] = iter -> pathname[j]; 
			}
			//memcpy(&exec_map[i], iter, sizeof(procmaps_struct));
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

//__attribute__((constructor))

void init(int argc, char **argv, char **envp)
{
	int len; 
	char print_buf[PRINT_SZ]; 

	on = 0;

	len = _snprintf(print_buf, PRINT_SZ, "\n\n******************\n\nRuntime constructor running...\nInital Memory Mapping:\n");
	_write(1, print_buf, len);

	pid_t pid = getpid();
	procmaps_struct *maps = pmparser_parse(pid); 
	num_exec_regions = save_exec_regions(maps);
	//mprotect(exec_map, sizeof(procmaps_struct) * num_exec_regions, PROT_READ);
	for(int i = 0; i < num_exec_regions; i++){
		len = _snprintf(print_buf, PRINT_SZ, "%d: %s 0x%lx-0x%lx\n", i, exec_map[i].pathname, (uintptr_t) exec_map[i].addr_start, (uintptr_t) exec_map[i].addr_end);
		_write(1, print_buf, len); 
	}
	len = _snprintf(print_buf, PRINT_SZ, "base address: 0x%lx\n", __base_addr);
	_write(1, print_buf, len);
	//uintptr_t return_addr = (uintptr_t) __builtin_return_address(0);
	//printf("CTOR: return address: 0x%lx\n", return_addr);
        //remap_code("start address", return_addr);	

	len = _snprintf(print_buf, PRINT_SZ, "\n\n******************\n\n");
	_write(1, print_buf, len);	

	ctor_done = 1;
	main_name = argv[0];
	//printf(main_name);
}
__attribute__((section(".init_array"))) void (*__init) (int, char**, char**) = &init;

/*
 * destructor which runs after the target program exits 
 * frees the text map 
 */ 

//__attribute__((destructor))
void fini()
{
	_write(1, "Runtime destructor running...\n", 30);
	if(exec_map != NULL){
		//free(exec_map);
	}
	remap_code(0, 1); 
}

__attribute__((section(".fini_array"))) void(*__fini) () = &fini;

/*
 * function which compares path to the list of regions which must 
 * always be executable
 * wrapper.so, vsyscall
 */ 

int always_exec(char *path){
	int i;
	for(i = 0; i < ALWAYS_EXEC_SIZE; i++){
		for(int j= 0; j < 600; j++){
			if(path[j] == '\0' && ALWAYS_EXEC[i][j] == '\0'){
				return 1; 
			}
			if(path[j] != ALWAYS_EXEC[i][j]){
				break; 
			}
		}
	}
	return 0;
}


int _strcmp(const char *a, const char *b) {
	  while (*a && *a == *b)
		      ++a, ++b;
	    return *a - *b;
}
/*
 * function which marks the code region containing symbol, executable
 *	marks all other executable code regions read only
 */

int remap_code(uintptr_t fun, int all_exec)
{
	int i;
	for(i = 0; i < num_exec_regions; i++)
	{

		int err;
		int len; 
		char print_buf[PRINT_SZ]; 

		len = _snprintf(print_buf, PRINT_SZ, "Remaping code sections. Target text 0x%lx...\n", fun); 
	       	procmaps_struct* maps = &exec_map[i];
	      
		len = _snprintf(print_buf, PRINT_SZ, "%s\n", maps -> pathname); 
		_write(1, print_buf, len); 
		//TODO restore original memory mapping
		uintptr_t mem_start =  (uintptr_t) maps -> addr_start;  
		uintptr_t mem_end = (uintptr_t) maps -> addr_end;  
	
		//wrapper code - KEEP EXEC!
		if(always_exec(maps -> pathname)){
			if(!all_exec){
				len = _snprintf(print_buf, PRINT_SZ, "CODE: 0x%lx-0x%lx ALWAYS EXEC\n", mem_start, mem_end); 
				_write(1, print_buf, len);
			}
		}
		else if(all_exec){
			err = _mprotect(maps->addr_start, maps->length, PROT_EXEC | PROT_READ);
		}
		//called function in memory region -- mark exec 
		else if(IN_RANGE((uintptr_t) mem_start, (uintptr_t) mem_end, (uintptr_t) fun))
		{
			//check if already marked EXEC
			if(maps -> is_x)
			{
				len = _snprintf(print_buf,
					       	PRINT_SZ, 
						"CODE: 0x%lx-0x%lx ALREADY marked EXEC, contains TARGET\n",
					       	mem_start, 
						mem_end); 
				_write(1, print_buf, len);
			}
			else
			{

				len = _snprintf(print_buf, 
						PRINT_SZ, 
						"CODE: 0x%lx-0x%lx marked NX, contains TARGET, marking it EXEC...\n", 
						mem_start, 
						mem_end); 
				_write(1, print_buf, len);
				err = _mprotect(maps->addr_start, maps->length, PROT_EXEC | PROT_READ);
				maps -> is_x = 1; 
				if(err)
				{
					len = _snprintf(print_buf, PRINT_SZ, "ERROR: mprotect PROT_READ | PROT_EXEC\n");
				       _write(1, print_buf, len);	
				}
			}
		}
		//check if memory region is executable -- mark read only
		else {

			//mark previously executable region as read only
			
		 	if(maps -> is_x){
				len = _snprintf(print_buf, PRINT_SZ, "CODE: 0x%lx-0x%lx marked EXEC, marking it NX\n", mem_start, mem_end);
				 _write(1, print_buf, len);
				err = _mprotect(maps -> addr_start, maps -> length, PROT_READ);
				maps -> is_x = 0;
				if(err){
					len = _snprintf(print_buf, PRINT_SZ, "ERROR: mprotect PROT_READ\n");
					_write(1, print_buf, len);	
				}
			}
			else{
				len = _snprintf(print_buf, PRINT_SZ, "CODE: 0x%lx-0x%lx marked NX\n", mem_start, mem_end);
				 _write(1, print_buf, len);
			}
		}
	}
	return 0; 
}

int is_main(uintptr_t return_addr){
	if(ctor_done){	
	int i;
	for(i = 0; i < num_exec_regions; i++){
		procmaps_struct *current = &exec_map[i]; 
		if(IN_RANGE((uintptr_t) current -> addr_start, (uintptr_t) current -> addr_end, (uintptr_t) return_addr)){
			if(_strcmp(main_name, current -> pathname) == 0){
				return 1; 
			}
			else{
				return 0;
			}
		}
	}
	return -1;
	}
	return 0;
}


/*
 * hook function for dynamic libraries, calls dynamic linker to resolve symbol
 * 	and then calls a function to handle the new memory mapping
 */

void *find_dyn_addr(const char* symbol, uintptr_t *return_addr)
{
	int was_on = 0;
	char print_buf[PRINT_SZ];
	if(_strcmp("exit", symbol) == 0){
		fini(); 
	}
	else if(on){
		on = 0; 
		was_on = 1; 
	}
	else if(is_main(*return_addr)){
		int len = _snprintf(print_buf, PRINT_SZ, "First call from main... starting runtime\n");
		_write(1, print_buf, len);
		was_on =1;
	}
       	if(was_on){
		int len = _snprintf(print_buf, PRINT_SZ, "Symbol: %s\n", symbol);
		_write(1, print_buf, len);
	}

	remap_code(0, 1); //DL_SYM only works on executable binaries -- POTENTIAL AVENUE OF ATTACK
	void *f = dlsym(RTLD_NEXT, symbol);
	int len = _snprintf(print_buf, PRINT_SZ, "Dlsym addr: 0x%lx\n", (uintptr_t) f);
	_write(1, print_buf, len);
	const char *err = dlerror();
	if(err != NULL)
	{
		int len = _snprintf(print_buf, PRINT_SZ, "%s\n", err);  
		_write(STDERR_FILENO, print_buf, len); 
		return NULL; 	
	}
	else
	{
		if(was_on){
			remap_code((uintptr_t) f, 0);
			on = 1;
		}
		return f; 
	}
}

int runtime_return(uintptr_t *return_addr){
       	if(on){
		on = 0;
		char print_buf[PRINT_SZ];
		int len = _snprintf(print_buf, PRINT_SZ, "Returning to address: 0x%lx...\n:", *return_addr);
		_write(1, print_buf, len);
		remap_code(*return_addr, 0);
		len = _snprintf(print_buf, PRINT_SZ, "Returning...\n");
		_write(1, print_buf, len);
		on = 1;
	}
	return 0; 
}

