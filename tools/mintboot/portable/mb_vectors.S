	.equ MB_CTX_SIZE, 72
	.equ MB_CTX_FRAME_OFF, 64

	.section .vectors,"a"
	.align 4
	.globl mb_vector_table
mb_vector_table:
	.long _mb_stack_top
	.long mb_entry

	.long mb_bus_error_stub
	.long mb_address_error_stub
	.long mb_illegal_instruction_stub
	.long mb_zero_divide_stub
	.long mb_chk_exception_stub
	.long mb_trapv_exception_stub
	.long mb_privilege_violation_stub
	.long mb_trace_exception_stub
	.long mb_line1010_stub
	.long mb_line1111_stub
	.long mb_reserved12_stub
	.long mb_reserved13_stub
	.long mb_reserved14_stub
	.long mb_reserved15_stub

	/* vectors 16-23: reserved */
	.rept 8
	.long mb_default_exception_stub
	.endr

	.long mb_spurious_stub
	.long mb_irq1_stub             /* irq1 */
	.long mb_irq2_stub             /* irq2 */
	.long mb_irq3_stub             /* irq3 */
	.long mb_irq4_stub             /* irq4 */
	.long mb_irq5_stub             /* irq5 */
	.long mb_irq6_stub             /* irq6 */
	.long mb_irq7_stub             /* irq7 */

	/* trap vectors 0-15 (vectors 32-47) */
	.long mb_default_exception_stub /* trap0 */
	.long mb_trap1_stub             /* trap1 */
	.long mb_default_exception_stub /* trap2 */
	.long mb_default_exception_stub /* trap3 */
	.long mb_default_exception_stub /* trap4 */
	.long mb_default_exception_stub /* trap5 */
	.long mb_default_exception_stub /* trap6 */
	.long mb_default_exception_stub /* trap7 */
	.long mb_default_exception_stub /* trap8 */
	.long mb_default_exception_stub /* trap9 */
	.long mb_default_exception_stub /* trap10 */
	.long mb_default_exception_stub /* trap11 */
	.long mb_default_exception_stub /* trap12 */
	.long mb_trap13_stub            /* trap13 */
	.long mb_trap14_stub            /* trap14 */
	.long mb_default_exception_stub /* trap15 */

	.rept 208
	.long mb_default_exception_stub
	.endr

	.text

	.globl mb_install_vector_base
	.type mb_install_vector_base, @function
mb_install_vector_base:
	lea mb_vector_table, %a0
	movec %a0, %vbr
	rts
	.size mb_install_vector_base, .-mb_install_vector_base

	.globl mb_trap1_stub
	.type mb_trap1_stub, @function
mb_trap1_stub:
	move.l %sp, %a0
	sub.l #MB_CTX_SIZE, %sp
	lea (%sp), %a1
	move.l %a0, (%a1)
	movem.l %d0-%d7/%a0-%a6, 4(%a1)
	move.w (%a0), MB_CTX_FRAME_OFF(%a1)
	move.l 2(%a0), MB_CTX_FRAME_OFF+2(%a1)
	move.w 6(%a0), MB_CTX_FRAME_OFF+6(%a1)
	move.l %sp, %a0
	move.l %a0, -(%sp)
	jsr mb_trap1_handler
	add.l #4, %sp
	lea (%sp), %a1
	movem.l 4(%a1), %d0-%d7/%a0-%a6
	add.l #MB_CTX_SIZE, %sp
	rte
	.size mb_trap1_stub, .-mb_trap1_stub

	.globl mb_trap13_stub
	.type mb_trap13_stub, @function
mb_trap13_stub:
	move.l %sp, %a0
	sub.l #MB_CTX_SIZE, %sp
	lea (%sp), %a1
	move.l %a0, (%a1)
	movem.l %d0-%d7/%a0-%a6, 4(%a1)
	move.w (%a0), MB_CTX_FRAME_OFF(%a1)
	move.l 2(%a0), MB_CTX_FRAME_OFF+2(%a1)
	move.w 6(%a0), MB_CTX_FRAME_OFF+6(%a1)
	move.l %sp, %a0
	move.l %a0, -(%sp)
	jsr mb_trap13_handler
	add.l #4, %sp
	lea (%sp), %a1
	movem.l 4(%a1), %d0-%d7/%a0-%a6
	add.l #MB_CTX_SIZE, %sp
	rte
	.size mb_trap13_stub, .-mb_trap13_stub

	.globl mb_trap14_stub
	.type mb_trap14_stub, @function
mb_trap14_stub:
	move.l %sp, %a0
	sub.l #MB_CTX_SIZE, %sp
	lea (%sp), %a1
	move.l %a0, (%a1)
	movem.l %d0-%d7/%a0-%a6, 4(%a1)
	move.w (%a0), MB_CTX_FRAME_OFF(%a1)
	move.l 2(%a0), MB_CTX_FRAME_OFF+2(%a1)
	move.w 6(%a0), MB_CTX_FRAME_OFF+6(%a1)
	move.l %sp, %a0
	move.l %a0, -(%sp)
	jsr mb_trap14_handler
	add.l #4, %sp
	lea (%sp), %a1
	movem.l 4(%a1), %d0-%d7/%a0-%a6
	add.l #MB_CTX_SIZE, %sp
	rte
	.size mb_trap14_stub, .-mb_trap14_stub


	.globl mb_spurious_stub
	.type mb_spurious_stub, @function
mb_spurious_stub:
	jsr mb_spurious_irq_handler
	rte
	.size mb_spurious_stub, .-mb_spurious_stub

	.macro MB_AUTOVEC_STUB name, handler
	.globl \name
	.type \name, @function
\name:
	movem.l %d0-%d1/%a0-%a1, -(%sp)
	jsr \handler
	movem.l (%sp)+, %d0-%d1/%a0-%a1
	rte
	.size \name, .-\name
	.endm

	MB_AUTOVEC_STUB mb_irq1_stub, mb_autovec_level1_handler
	MB_AUTOVEC_STUB mb_irq2_stub, mb_autovec_level2_handler
	MB_AUTOVEC_STUB mb_irq3_stub, mb_autovec_level3_handler
	MB_AUTOVEC_STUB mb_irq4_stub, mb_autovec_level4_handler
	MB_AUTOVEC_STUB mb_irq5_stub, mb_autovec_level5_handler
	MB_AUTOVEC_STUB mb_irq6_stub, mb_autovec_level6_handler
	MB_AUTOVEC_STUB mb_irq7_stub, mb_autovec_level7_handler

	.macro MB_EXC_STUB name, ret
	.globl \name
	.type \name, @function
\name:
	move.l %sp, %a0
	sub.l #MB_CTX_SIZE, %sp
	lea (%sp), %a1
	move.l %a0, (%a1)
	movem.l %d0-%d7/%a0-%a6, 4(%a1)
	move.w (%a0), MB_CTX_FRAME_OFF(%a1)
	move.l 2(%a0), MB_CTX_FRAME_OFF+2(%a1)
	move.w 6(%a0), MB_CTX_FRAME_OFF+6(%a1)
	move.l %sp, %a0
	move.l %a0, -(%sp)
	jsr mb_default_exception_handler
	add.l #4, %sp
	lea (%sp), %a1
	movem.l 4(%a1), %d0-%d7/%a0-%a6
	add.l #MB_CTX_SIZE, %sp
\ret
	.size \name, .-\name
	.endm

	.globl mb_default_exception_stub
	.type mb_default_exception_stub, @function
mb_default_exception_stub:
	move.l %sp, %a0
	sub.l #MB_CTX_SIZE, %sp
	lea (%sp), %a1
	move.l %a0, (%a1)
	movem.l %d0-%d7/%a0-%a6, 4(%a1)
	move.w (%a0), MB_CTX_FRAME_OFF(%a1)
	move.l 2(%a0), MB_CTX_FRAME_OFF+2(%a1)
	move.w 6(%a0), MB_CTX_FRAME_OFF+6(%a1)
	move.l %sp, %a0
	move.l %a0, -(%sp)
	jsr mb_default_exception_handler
	add.l #4, %sp
	add.l #MB_CTX_SIZE, %sp
1:
	bra 1b
	.size mb_default_exception_stub, .-mb_default_exception_stub

	MB_EXC_STUB mb_bus_error_stub, rte
	MB_EXC_STUB mb_address_error_stub, rte
	MB_EXC_STUB mb_illegal_instruction_stub, rte
	MB_EXC_STUB mb_zero_divide_stub, rte
	MB_EXC_STUB mb_chk_exception_stub, rte
	MB_EXC_STUB mb_trapv_exception_stub, rte
	MB_EXC_STUB mb_privilege_violation_stub, rte
	MB_EXC_STUB mb_trace_exception_stub, rte
	MB_EXC_STUB mb_line1010_stub, rte
	MB_EXC_STUB mb_line1111_stub, rte
	MB_EXC_STUB mb_reserved12_stub, rte
	MB_EXC_STUB mb_reserved13_stub, rte
	MB_EXC_STUB mb_reserved14_stub, rte
	MB_EXC_STUB mb_reserved15_stub, rte

	.section .note.GNU-stack,"",@progbits
