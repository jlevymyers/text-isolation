#define _GNU_SOURCE

#include <dlfcn.h>
#include <link.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <asm/unistd.h>
#include <stddef.h>
#include "tinyprintf/tinyprintf.h"
#include "tinyprintf/tinyprintf.c"

#include "find_dyn_addr.h"

#include <link.h>


const char* const DLWRAP_PATH = "/gpfs/main/home/jlevymye/course/cs2951/text-isolation/wrapper.so";
const char* const DL_PATH  = "/lib/x86-64-linux-gnu/libdl-2.24.so";
const char* const VSYSCALL ="[vsyscall]";

#define IN_RANGE(a, b, c) (a <= c) && (c < b)

int on = 0;

#define EXEC_LIST_MAX 1024 
exec_segment_t* exec_list_head = NULL;
int exec_list_size = 0;
int executable_array[1024]; 

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

/* 
 * system call wrappers functions 
 * needed to avoid using external libraries
 */ 

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
 * https://github.com/GaloisInc/minlibc/blob/master/strncpy.c
 */

char  *strncpy(char *dest, const char *src, size_t count)
{
	char *tmp = dest;
	while (count-- && (*dest++ = *src++) != '\0')
	/* nothing */;
	return tmp;
}

/*
 * allocates memory for a new segment representing an executable segment
 */ 

exec_segment_t *add_segment(uintptr_t vaddr_start, size_t size, const char *name, exec_segment_t **exec_list){
	char print_buf[PRINT_SZ];
	if(exec_list == NULL){
		int len = tfp_snprintf(print_buf, PRINT_SZ, "ERROR: Invalid Pointer\n");
		_write(1, print_buf, len);
		return NULL;
	}
	
	exec_segment_t *segment= (exec_segment_t *) _mmap(NULL, sizeof(exec_segment_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); 
	if(segment == ((void *) -1)){
		int len = tfp_snprintf(print_buf, PRINT_SZ, "ERROR: mmap failed. could not create segement\n");
		_write(1, print_buf, len);
		return NULL; 
	}
	
	segment -> vaddr_start = vaddr_start;
	segment -> vaddr_end = vaddr_start + size; 
	segment -> size = size; 
	segment -> next = NULL;
	strncpy(segment -> name, name, DTI_NAME_SZ);
	//give the executable segment a pointer to its (sometimes) modifiable execuatable bit
	segment -> executable = executable_array + exec_list_size;	

	if(*exec_list == NULL){
		*exec_list = segment; 
		exec_list_size++; 
		
		int len = tfp_snprintf(print_buf, PRINT_SZ, "[%d]Successfully created new segment at head of list, address 0x%x\n", exec_list_size - 1, segment);
		_write(1, print_buf, len);
		
		return segment; 
	}

	exec_segment_t *current = *exec_list; 
	while(current -> next != NULL){
		current = current -> next; 
	}
	current -> next = segment;
        exec_list_size++;

	int len = tfp_snprintf(print_buf, PRINT_SZ, "[%d]Successfully created new segment\n", exec_list_size - 1);
	_write(1, print_buf, len);
	return segment;	
}


static int
callback(struct dl_phdr_info *info, size_t size, void *data)
{
	char print_buf[PRINT_SZ]; 
	int j;
	int len = tfp_snprintf(print_buf, PRINT_SZ, "Name: %s, Executable Segments:\n", info->dlpi_name,
		info->dlpi_phnum);
		_write(1, print_buf, len);
	for (j = 0; j < info->dlpi_phnum; j++){
		Elf64_Phdr segment = info -> dlpi_phdr[j];
		if((segment.p_flags & PF_X) && (segment.p_type == PT_LOAD)){
			void *p_addr = (void *) info -> dlpi_addr + segment.p_vaddr; 
			len = tfp_snprintf(print_buf, PRINT_SZ, "\t [%d] Address: 0x%lx-0x%lx\n",
				j, (void *) p_addr, p_addr + segment.p_memsz );
			_write(1, print_buf, len);
		       	//add to our list of executable segment
			exec_segment_t* result = add_segment((uintptr_t) p_addr, segment.p_memsz, info -> dlpi_name, &exec_list_head);
			if(result == NULL){
				len = tfp_snprintf(print_buf, PRINT_SZ, "ERROR: Creating Exec Segment Stucture\n");
				_write(1, print_buf, len);
			}	
		}
	}
	return 0;
}

/*
 * constructor for runtime system that executes before the program
 * creates an initial mapping of the text regions, and marks it read only
 */ 

void init(uintptr_t main_addr)
{
	int len; 
	char print_buf[PRINT_SZ]; 

	on = 0;

	len = tfp_snprintf(print_buf, PRINT_SZ, "\n\n******************\n\nRuntime constructor running...\nInitial Memory Mapping:\n");
	_write(1, print_buf, len);

	//pid_t pid = getpid();
	//procmaps_struct *maps = pmparser_parse(pid); 
	//num_exec_regions = save_exec_regions(maps);
	//mprotect(exec_map, sizeof(procmaps_struct) * num_exec_regions, PROT_READ);
	
	/*for(int i = 0; i < num_exec_regions; i++){
		len = tfp_snprintf(print_buf, PRINT_SZ, "%d: %s 0x%lx-0x%lx\n", i, exec_map[i].pathname, (uintptr_t) exec_map[i].addr_start, (uintptr_t) exec_map[i].addr_end);
		_write(1, print_buf, len); 
	}
	*/
     
       	//create executable list structure from dl_phdr	
       	dl_iterate_phdr(callback, (void *) exec_list_head);	

	len = tfp_snprintf(print_buf, PRINT_SZ, "\n\n******************\n\n");
	_write(1, print_buf, len);

	len = tfp_snprintf(print_buf, PRINT_SZ, "Marking non-main code NX. Main symbol at address %lx\n", main_addr);
	_write(1, print_buf, len);
	remap_code(main_addr, 0);	
	ctor_done = 1;
}

/*
 * destructor which runs after the target program exits 
 * frees the text map 
 */ 

void fini()
{
	_write(1, "Runtime destructor running...\n", 30);
	if(exec_list_head != NULL){
		//free(exec_map);
	}
	remap_code(0, 1); 
}

/*
 * function which compares path to the list of regions which must 
 * 	always be executable such as: 
 * 	wrapper.so, vsyscall, ld.so
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

/*
 * implements basic libc strcmp operations
 */ 

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
	int len; 
	char print_buf[PRINT_SZ]; 
	len = tfp_snprintf(print_buf, PRINT_SZ, "Remaping code sections. Target text 0x%lx...\n", fun); 

	exec_segment_t *segment = exec_list_head;
	while(segment != NULL)
	{
		int err;
			      
		uintptr_t mem_start =  segment -> vaddr_start;  
		uintptr_t mem_end = segment -> vaddr_end;;  
		size_t mem_size = segment -> size;
		char *mem_name = segment -> name;
		int is_exec = *segment -> executable;

		//wrapper code - KEEP EXEC!
		if(always_exec(mem_name)){
			if(!is_exec){
				len = tfp_snprintf(print_buf, PRINT_SZ, "CODE: 0x%lx-0x%lx ALWAYS EXEC\n", mem_start, mem_end); 
				_write(1, print_buf, len);
			}
		}
		else if(all_exec){
			if(!is_exec){
				err = _mprotect((void*) mem_start, mem_size, PROT_EXEC | PROT_READ);
				is_exec = 1;

				len = tfp_snprintf(print_buf,
					       	PRINT_SZ, 
						"CODE: 0x%lx-0x%lx Marking all code exec\n",
					       	mem_start, 
						mem_end); 
				_write(1, print_buf, len);
			}
		}
		//called function in memory region -- mark exec 
		else if(IN_RANGE( mem_start,  mem_end, fun)){
			len = tfp_snprintf(print_buf, PRINT_SZ, "%s\n", segment -> name); 
			_write(1, print_buf, len); 
			//check if already marked EXEC
			if(*(segment -> executable)){
				len = tfp_snprintf(print_buf,
					       	PRINT_SZ, 
						"CODE: 0x%lx-0x%lx ALREADY marked EXEC, contains TARGET\n",
					       	mem_start, 
						mem_end); 
				_write(1, print_buf, len);
			}
			else{
				len = tfp_snprintf(print_buf, 
						PRINT_SZ, 
						"CODE: 0x%lx-0x%lx marked NX, contains TARGET, marking it EXEC...\n", 
						mem_start, 
						mem_end); 
				_write(1, print_buf, len);
				err = _mprotect((void*) mem_start, mem_size, PROT_EXEC | PROT_READ);
				is_exec = 1; 
				if(err)
				{
					len = tfp_snprintf(print_buf, PRINT_SZ, "ERROR: mprotect PROT_READ | PROT_EXEC\n");
				       _write(1, print_buf, len);	
				}
			}
		}
		//check if memory region is executable -- mark read only
		else {
			len = tfp_snprintf(print_buf, PRINT_SZ, "%s\n", mem_name); 
			_write(1, print_buf, len); 

			//mark previously executable region as read only
		 	if(*(segment -> executable)){
				len = tfp_snprintf(print_buf, PRINT_SZ, "CODE: 0x%lx-0x%lx marked EXEC, marking it NX\n", mem_start, mem_end);
				 _write(1, print_buf, len);
				err = _mprotect((void*) mem_start, mem_size, PROT_READ);
				is_exec = 0;
				if(err){
					len = tfp_snprintf(print_buf, PRINT_SZ, "ERROR: mprotect PROT_READ\n");
					_write(1, print_buf, len);	
				}
			}
			else{
				len = tfp_snprintf(print_buf, PRINT_SZ, "CODE: 0x%lx-0x%lx marked NX\n", mem_start, mem_end);
				 _write(1, print_buf, len);
			}
		}
		segment = segment -> next;
	}
	return 0; 
}

typedef int (*main_t)(int, const char**, char**);

main_t main_addr = NULL;

/*
 * function whose address is returned when an error is found resolving a symbol
 */

int _dti_error(){
	_write(1, "ERROR: Resolving Symbol. Exiting Process...\n", 7);
	return -1;
}

/*
 * is passed as an argument to _libc_start_main. starts the runtime protection
 * and calls the main function. turn off runtime protection when main terminates
 */ 

int dti_main(int argc, const char* argv[], char *envp[]){
	char print_buf[PRINT_SZ];
	int len = tfp_snprintf(print_buf, PRINT_SZ, "Call to main sucessfully redirected\n");
	_write(1, print_buf, len);

	len = tfp_snprintf(print_buf, PRINT_SZ, "Calling runtime constructor...\n");
	_write(1, print_buf, len);
	init((uintptr_t) main_addr);

	len = tfp_snprintf(print_buf, PRINT_SZ, "Calling main at address: %lx...\n", main_addr);
	_write(1, print_buf, len);

	on = 1;	
	int result = (*main_addr)(argc, argv, envp);
	on = 0;

	len = tfp_snprintf(print_buf, PRINT_SZ, "Return from main with value: %d\n", result);
	_write(1, print_buf, len);
	fini();

	len = tfp_snprintf(print_buf, PRINT_SZ, "Calling runtime destructor...\n");
	_write(1, print_buf, len);

	return result;
}



/*
 * hook function for dynamic libraries, calls dynamic linker to resolve symbol
 * 	and then calls a function to handle the new memory mapping
 */

void *find_dyn_addr(const char* symbol, uintptr_t *return_addr, uintptr_t *rdi)
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
	else if(!_strcmp("__libc_start_main", symbol)){
		int len = tfp_snprintf(print_buf, PRINT_SZ, "Call to libc main initialization function\n");
		_write(1, print_buf, len);
		void *f = dlsym(RTLD_NEXT, symbol);
		main_addr = (main_t) (*rdi); 
		len = tfp_snprintf(print_buf, PRINT_SZ, "Main at: 0x%lx, Redirecting main to: 0x%lx, Stack Location 0x%lx\n", *rdi, (uintptr_t) &dti_main, rdi);
		_write(1, print_buf, len);
		*rdi = (uintptr_t) &dti_main;
		return f; 
		
	}
       	
	if(was_on){
		int len = tfp_snprintf(print_buf, PRINT_SZ, "Symbol: %s\n", symbol);
		_write(1, print_buf, len);
	}

	remap_code(0, 1); //DL_SYM only works on executable binaries -- POTENTIAL AVENUE OF ATTACK
	void *f = dlsym(RTLD_NEXT, symbol);
	int len = tfp_snprintf(print_buf, PRINT_SZ, "Dynamic Symbol %s, Address: 0x%lx\n", symbol, (uintptr_t) f);
	_write(1, print_buf, len);
	const char *err = dlerror();
	if(err != NULL)
	{
		int len = tfp_snprintf(print_buf, PRINT_SZ, "ERROR: DLOPEN\n\t%s\n", err);  
		_write(1, print_buf, len); 
		return &_dti_error; 	
	}
	else
	{
		if(was_on){
			remap_code((uintptr_t) f, 0);
			on = 1;
		}
		else{
			printf("WARNING: Returning with all code marked EXEC\n");
		}
		return f; 
	}
}

/*
 * called on return from dynamic call. restored return address section to EXEC
 */ 

int runtime_return(uintptr_t *return_addr){
       	if(on){
		on = 0;
		char print_buf[PRINT_SZ];
		int len = tfp_snprintf(print_buf, PRINT_SZ, "Returning to address: 0x%lx...\n", *return_addr);
		_write(1, print_buf, len);
		remap_code(*return_addr, 0);
		len = tfp_snprintf(print_buf, PRINT_SZ, "Returning...\n\n");
		_write(1, print_buf, len);
		on = 1;
	}
	return 0; 
}

