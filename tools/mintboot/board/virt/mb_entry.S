	.text
	.globl mb_entry
	.type mb_entry, @function

mb_entry:
	/* Set up a temporary stack and jump into C. */
	lea _mb_stack_top, %sp
	jsr mb_virt_start

1:
	bra 1b

	.size mb_entry, .-mb_entry

	.bss
	.align 4
	.globl _mb_stack
_mb_stack:
	.space 4096
	.globl _mb_stack_top
_mb_stack_top:
	.space 4

	.section .note.GNU-stack,"",@progbits
