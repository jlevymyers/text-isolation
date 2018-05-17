#define DTI_NAME_SZ 600 

typedef struct exec_segment {
	uintptr_t vaddr_start;
       	uintptr_t vaddr_end; 	
	size_t size; 
        char name [DTI_NAME_SZ];
	struct exec_segment *next;
	int *executable;	
} exec_segment_t;


void *
find_dyn_addr(const char *symbol, uintptr_t *return_addr, uintptr_t *rdi);

int
runtime_return(uintptr_t *return_addr);
