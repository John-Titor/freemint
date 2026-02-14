	.set MB_SUP_STACK_SIZE, 16384
	.set MB_USR_STACK_SIZE, 16384

	.text
	.globl mb_entry
	.type mb_entry, @function

mb_entry:
	/* Set up explicit supervisor/user stacks and jump into C. */
	lea _mb_super_stack_top, %sp
	lea _mb_user_stack_top, %a0
	move.l %a0, %usp
	jsr mb_common_start

1:
	bra 1b

	.size mb_entry, .-mb_entry

	.bss
	.align 4
	.globl _mb_super_stack
_mb_super_stack:
	.space MB_SUP_STACK_SIZE
	.globl _mb_super_stack_top
_mb_super_stack_top:
	.globl _mb_stack_top
_mb_stack_top:
	/* Compatibility alias used by the initial vector table. */
	.globl _mb_user_stack
_mb_user_stack:
	.space MB_USR_STACK_SIZE
	.globl _mb_user_stack_top
_mb_user_stack_top:


	.section .note.GNU-stack,"",@progbits
