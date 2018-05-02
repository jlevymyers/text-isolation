#include "find_dyn_addr.h"
.globl __base_addr
__base_addr:
	 .quad 0x650 
/*
  *	Dynamic Symbol Wrapper For: write
*/
write:
	.globl	write
	.type	write,@function
	movq %rsp, %r8
	pushq %rbp
	movq %rsp, %rbp
	pushq %rdi
	push %rsi
	pushq %rdx
	pushq %rcx
	pushq %r8
	push %r9
	push %r10
	push %r11
	leaq .name0(%rip), %rdi
	movq %r8, %rsi
	callq find_dyn_addr@PLT
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rcx
	popq %rdx
	pop %rsi
	popq %rdi
	callq *%rax
	pushq %rdi
	push %rsi
	pushq %rdx
	pushq %rcx
	pushq %r8
	push %r9
	movq %r8, %rdi
	push %rax
	callq runtime_return@PLT
	pop %rax
	popq %r9
	popq %r8
	popq %rcx
	popq %rdx
	pop %rsi
	popq %rdi
	popq %rbp
	retq
.name0:
	.asciz	 "write"
	.size	.name0, 5
/*
  *	Dynamic Symbol Wrapper For: close
*/
close:
	.globl	close
	.type	close,@function
	movq %rsp, %r8
	pushq %rbp
	movq %rsp, %rbp
	pushq %rdi
	push %rsi
	pushq %rdx
	pushq %rcx
	pushq %r8
	push %r9
	push %r10
	push %r11
	leaq .name1(%rip), %rdi
	movq %r8, %rsi
	callq find_dyn_addr@PLT
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rcx
	popq %rdx
	pop %rsi
	popq %rdi
	callq *%rax
	pushq %rdi
	push %rsi
	pushq %rdx
	pushq %rcx
	pushq %r8
	push %r9
	movq %r8, %rdi
	push %rax
	callq runtime_return@PLT
	pop %rax
	popq %r9
	popq %r8
	popq %rcx
	popq %rdx
	pop %rsi
	popq %rdi
	popq %rbp
	retq
.name1:
	.asciz	 "close"
	.size	.name1, 5
/*
  *	Dynamic Symbol Wrapper For: read
*/
read:
	.globl	read
	.type	read,@function
	movq %rsp, %r8
	pushq %rbp
	movq %rsp, %rbp
	pushq %rdi
	push %rsi
	pushq %rdx
	pushq %rcx
	pushq %r8
	push %r9
	push %r10
	push %r11
	leaq .name2(%rip), %rdi
	movq %r8, %rsi
	callq find_dyn_addr@PLT
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rcx
	popq %rdx
	pop %rsi
	popq %rdi
	callq *%rax
	pushq %rdi
	push %rsi
	pushq %rdx
	pushq %rcx
	pushq %r8
	push %r9
	movq %r8, %rdi
	push %rax
	callq runtime_return@PLT
	pop %rax
	popq %r9
	popq %r8
	popq %rcx
	popq %rdx
	pop %rsi
	popq %rdi
	popq %rbp
	retq
.name2:
	.asciz	 "read"
	.size	.name2, 4
/*
  *	Dynamic Symbol Wrapper For: open
*/
open:
	.globl	open
	.type	open,@function
	movq %rsp, %r8
	pushq %rbp
	movq %rsp, %rbp
	pushq %rdi
	push %rsi
	pushq %rdx
	pushq %rcx
	pushq %r8
	push %r9
	push %r10
	push %r11
	leaq .name3(%rip), %rdi
	movq %r8, %rsi
	callq find_dyn_addr@PLT
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rcx
	popq %rdx
	pop %rsi
	popq %rdi
	callq *%rax
	pushq %rdi
	push %rsi
	pushq %rdx
	pushq %rcx
	pushq %r8
	push %r9
	movq %r8, %rdi
	push %rax
	callq runtime_return@PLT
	pop %rax
	popq %r9
	popq %r8
	popq %rcx
	popq %rdx
	pop %rsi
	popq %rdi
	popq %rbp
	retq
.name3:
	.asciz	 "open"
	.size	.name3, 4
