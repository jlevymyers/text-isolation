//#include <stdint.h>

void *
find_dyn_addr(const char *symbol, uintptr_t *return_addr);

int
runtime_return(uintptr_t *return_addr);
