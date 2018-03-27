#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "generate_asm.h"

/*
 * generates the beginning of an assembly file
 *
 */

size_t start_asm_file(char *target)
{
	asm_counter = 0;

	size_t len = sprintf(target, "#include \"find_dyn_addr.h\"\n"); 
	
	//mark stack non-exec
	//len += sprintf(target + len, ".section .note.GNU-stack, \"\", %%progbits\n");
	return len; 
}

/*
 * generates hook for the given symbol 
 *
 */ 

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

	//%rdi, %rsi, %rdx, %rcx, %r8, %r9 caller registers

	offset += sprintf(target + offset, "/*\n  *\tDynamic Symbol Wrapper For: %s\n*/\n", symbol);

	//function label
	offset += sprintf(target + offset, "%s:\n", symbol);
	offset += sprintf(target + offset, "\t.globl\t%s\n", symbol);
	offset += sprintf(target + offset, "\t.type\t%s,@function\n", symbol);

	//prologue -- TODO save all registers
	offset += sprintf(target + offset, "\tpushq %%rbp\n");
	offset += sprintf(target + offset, "\tmovq %%rsp, %%rbp\n");

	offset += sprintf(target + offset, "\tpushq %%rdi\n");
	offset += sprintf(target + offset, "\tpush %%rsi\n");
	offset += sprintf(target + offset, "\tpushq %%rdx\n");
	offset += sprintf(target + offset, "\tpushq %%rcx\n");
	offset += sprintf(target + offset, "\tpushq %%r8\n");
	offset += sprintf(target + offset, "\tpush %%r9\n");

	//call hook 
	offset += sprintf(target + offset, "\tleaq .name%x(%%rip), %%rdi\n", asm_counter);
	offset += sprintf(target + offset, "\tcallq find_dyn_addr@PLT\n");

	//epilogue -- TODO restore all registers 
	offset += sprintf(target + offset, "\tpopq %%r9\n");
	offset += sprintf(target + offset, "\tpopq %%r8\n");
	offset += sprintf(target + offset, "\tpopq %%rcx\n");
	offset += sprintf(target + offset, "\tpopq %%rdx\n");
	offset += sprintf(target + offset, "\tpop %%rsi\n");
	offset += sprintf(target + offset, "\tpopq %%rdi\n");

	//call target function 
	offset += sprintf(target + offset, "\tcallq *%%rax\n");
	offset += sprintf(target + offset, "\tpopq %%rbp\n");
	offset += sprintf(target + offset, "\tretq\n");

	//symbol string 
	offset += sprintf(target + offset, ".name%x:\n", asm_counter); 
	offset += sprintf(target + offset, "\t.asciz\t \"%s\"\n", symbol);
	offset += sprintf(target + offset, "\t.size\t.name%x, %lu\n", asm_counter, len);
	asm_counter++; 	
	return offset; 
}
