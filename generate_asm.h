#include <stdint.h>

size_t
generate_asm(const char *symbol, char *target, int uid);

size_t
start_asm_file(char *target, uintptr_t base_addr);

int asm_counter; 
