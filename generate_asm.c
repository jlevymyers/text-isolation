#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "generate_asm.h"

size_t start_asm_file(char *target)
{
	asm_counter = 0;

	size_t len = sprintf(target, "#include \"find_dyn_addr.h\"\n"); 
	return len; 
}

size_t generate_asm(const char *symbol, char *target)
{

	if(symbol == NULL){
		return -1; 
	}

	size_t len = strlen(symbol); 
	if(len < 1){
		return -1;
	}
	if(symbol[0] == '_'){
		return 0;
	}

	size_t offset = 0;

	offset += sprintf(target + offset, "/*\n  *\tDynamic Symbol Wrapper For: %s\n*/\n", symbol);
	offset += sprintf(target + offset, "%s:\n", symbol);
	offset += sprintf(target + offset, "\tpushq %%rbp\n");

	offset += sprintf(target + offset, "\t.globl\t%s\n", symbol);
	offset += sprintf(target + offset, "\t.type\t%s,@function\n", symbol);

	offset += sprintf(target + offset,"\tmovq %%rsp, %%rbp\n");
	offset += sprintf(target + offset,"\tpushq %%rdi\n");
	offset += sprintf(target + offset,"\tpush %%rsi\n");
	offset += sprintf(target + offset,"\tleaq .name%x(%%rip), %%rdi\n", asm_counter);
	offset += sprintf(target + offset,"\tcallq find_dyn_addr@PLT\n");
	offset += sprintf(target + offset,"\tpop %%rsi\n");
	offset += sprintf(target + offset,"\tpopq %%rdi\n");
	offset += sprintf(target + offset,"\tcallq *%%rax\n");
	offset += sprintf(target + offset,"\tpopq %%rbp\n");
	offset += sprintf(target + offset,"\tretq\n");
	offset += sprintf(target + offset,".name%x:\n", asm_counter); 
	offset += sprintf(target + offset, "\t.asciz\t \"%s\"\n", symbol);
	offset += sprintf(target + offset, "\t.size\t.name%x, %lu\n", asm_counter, len);
	asm_counter++; 	
	return offset; 
}
