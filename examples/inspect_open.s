	.text
	.file	"inspect_open.c"
	.globl	open
	.align	16, 0x90
	.type	open,@function
open:                                   # @open
# BB#0:
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rdi
	push	%rsi
	
	leaq	.L.str(%rip), %rdi
	callq	find_dym_addr@PLT
	pop 	%rsi
	popq 	%rdi

	callq	*%rax
	popq	%rbp
	retq
.Lfunc_end0:
	.size	open, .Lfunc_end0-open

	.type	.L.str,@object          # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
.L.str:
	.asciz	"open"
	.size	.L.str, 5


	.ident	"clang version 3.8.1-24 (tags/RELEASE_381/final)"
	.section	".note.GNU-stack","",@progbits
