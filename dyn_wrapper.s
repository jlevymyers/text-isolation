#include "find_dyn_addr.h"
/*
  *	Dynamic Symbol Wrapper For: printf
*/
printf:
	pushq %rbp
	.globl	printf
	.type	printf,@function
	movq %rsp, %rbp
	pushq %rdi
	push %rsi
	leaq .name0(%rip), %rdi
	callq find_dyn_addr@PLT
	pop %rsi
	popq %rdi
	callq *%rax
	popq %rbp
	retq
.name0:
	.asciz	 "printf"
	.size	.name0, 6
/*
  *	Dynamic Symbol Wrapper For: close
*/
close:
	pushq %rbp
	.globl	close
	.type	close,@function
	movq %rsp, %rbp
	pushq %rdi
	push %rsi
	leaq .name1(%rip), %rdi
	callq find_dyn_addr@PLT
	pop %rsi
	popq %rdi
	callq *%rax
	popq %rbp
	retq
.name1:
	.asciz	 "close"
	.size	.name1, 5
/*
  *	Dynamic Symbol Wrapper For: read
*/
read:
	pushq %rbp
	.globl	read
	.type	read,@function
	movq %rsp, %rbp
	pushq %rdi
	push %rsi
	leaq .name2(%rip), %rdi
	callq find_dyn_addr@PLT
	pop %rsi
	popq %rdi
	callq *%rax
	popq %rbp
	retq
.name2:
	.asciz	 "read"
	.size	.name2, 4
/*
  *	Dynamic Symbol Wrapper For: open
*/
open:
	pushq %rbp
	.globl	open
	.type	open,@function
	movq %rsp, %rbp
	pushq %rdi
	push %rsi
	leaq .name3(%rip), %rdi
	callq find_dyn_addr@PLT
	pop %rsi
	popq %rdi
	callq *%rax
	popq %rbp
	retq
.name3:
	.asciz	 "open"
	.size	.name3, 4
