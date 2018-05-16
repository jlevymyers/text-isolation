#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include "generate_asm.h"
/*
 * generates the beginning of an assembly file
 *
 */

size_t start_asm_file(char *target, uintptr_t base_addr) 
{
	asm_counter = 0;

	size_t len = sprintf(target, "#include \"find_dyn_addr.h\"\n"); 
	len += sprintf(target + len, ".globl __base_addr\n");
	len += sprintf(target + len, "__base_addr:\n");
	len += sprintf(target + len, "\t .quad 0x%lx \n", base_addr);
	
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
	//if(symbol[0] == '_' &&  symbol[1] == '_'){
//		return 0;
//	}

	size_t offset = 0;

	//%rdi, %rsi, %rdx, %rcx, %r8, %r9 caller registers

	offset += sprintf(target + offset, "/*\n  *\tDynamic Symbol Wrapper For: %s\n*/\n", symbol);

	//function label
	offset += sprintf(target + offset, "%s:\n", symbol);
	offset += sprintf(target + offset, "\t.globl\t%s\n", symbol);
	offset += sprintf(target + offset, "\t.type\t%s,@function\n", symbol);

	//prologue -- TODO save all registers
	offset += sprintf(target + offset, "\tmovq %%rsp, %%r8\n");
	offset += sprintf(target + offset, "\tpushq %%rbp\n");
	offset += sprintf(target + offset, "\tmovq %%rsp, %%rbp\n");

	offset += sprintf(target + offset, "\tpush %%rsi\n");
	offset += sprintf(target + offset, "\tpushq %%rdx\n");
	offset += sprintf(target + offset, "\tpushq %%rcx\n");
	offset += sprintf(target + offset, "\tpushq %%r8\n");
	offset += sprintf(target + offset, "\tpush %%r9\n");
	offset += sprintf(target + offset, "\tpush %%r10\n");
	offset += sprintf(target + offset, "\tpush %%r11\n");

	//save address of %rdi saved register to modify arguments to __libc_start_main	
	offset += sprintf(target + offset, "\tpushq %%rdi\n");
	offset += sprintf(target + offset, "\tmovq %%rsp, %%rdx\n");
	
	//get return address 
	//offset += sprintf(target + offset, "\tmovq $0, %%rdi\n"); //return level 0
	//offset += sprintf(target + offset, "\t callq __builtin_return_address@PLT\n");
	//offset += sprintf(target + offset, "\t movq %%rax, %%rsi\n");

	//call hook 
	offset += sprintf(target + offset, "\tleaq .name%x(%%rip), %%rdi\n", asm_counter);
	offset += sprintf(target + offset, "\tmovq %%r8, %%rsi\n");
	offset += sprintf(target + offset, "\tcallq find_dyn_addr@PLT\n");

	//epilogue -- TODO restore all registers 

	offset += sprintf(target + offset, "\tpopq %%rdi\n");
	
	offset += sprintf(target + offset, "\tpopq %%r11\n");
	offset += sprintf(target + offset, "\tpopq %%r10\n");
	offset += sprintf(target + offset, "\tpopq %%r9\n");
	offset += sprintf(target + offset, "\tpopq %%r8\n");
	offset += sprintf(target + offset, "\tpopq %%rcx\n");
	offset += sprintf(target + offset, "\tpopq %%rdx\n");
	offset += sprintf(target + offset, "\tpop %%rsi\n");

	//call target function 
	offset += sprintf(target + offset, "\tcallq *%%rax\n");


	//save registers again
	offset += sprintf(target + offset, "\tpushq %%rdi\n");
	offset += sprintf(target + offset, "\tpush %%rsi\n");
	offset += sprintf(target + offset, "\tpushq %%rdx\n");
	offset += sprintf(target + offset, "\tpushq %%rcx\n");
	offset += sprintf(target + offset, "\tpushq %%r8\n");
	offset += sprintf(target + offset, "\tpush %%r9\n");

	//set return address argument
	offset += sprintf(target + offset, "\tmovq %%r8, %%rdi\n");

	//save target return argument
	offset += sprintf(target + offset, "\tpush %%rax\n");
	
	offset += sprintf(target + offset, "\tcallq runtime_return@PLT\n");
	
	//restore target return argument 
	offset += sprintf(target + offset, "\tpop %%rax\n");
	//restore registers 
	offset += sprintf(target + offset, "\tpopq %%r9\n");
	offset += sprintf(target + offset, "\tpopq %%r8\n");
	offset += sprintf(target + offset, "\tpopq %%rcx\n");
	offset += sprintf(target + offset, "\tpopq %%rdx\n");
	offset += sprintf(target + offset, "\tpop %%rsi\n");
	offset += sprintf(target + offset, "\tpopq %%rdi\n");

	//restore target function return value
	offset += sprintf(target + offset, "\tpopq %%rbp\n");
	offset += sprintf(target + offset, "\tretq\n");

	//symbol string 
	offset += sprintf(target + offset, ".name%x:\n", asm_counter); 
	offset += sprintf(target + offset, "\t.asciz\t \"%s\"\n", symbol);
	offset += sprintf(target + offset, "\t.size\t.name%x, %lu\n", asm_counter, len);
	asm_counter++; 	
	return offset; 
}
