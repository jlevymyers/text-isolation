	.globl	open
	.type	open,@function
open:                                   # @open
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

.L.str:
	.asciz	"open"
	.size	.L.str, 5
