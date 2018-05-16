//#include <stdint.h>

void *
find_dyn_addr(const char *symbol, uintptr_t *return_addr, uintptr_t *rdi);

int
runtime_return(uintptr_t *return_addr);
