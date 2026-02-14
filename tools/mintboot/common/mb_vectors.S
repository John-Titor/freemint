	.text

	/*
	 * Minmal line-A handler, only supports A000.
	 */
	.globl mb_line1010
	.globl mb_linea_struct_ptr
	.type mb_line1010, @function
mb_line1010:
	move.l		2(%sp),%a0						// return address
	clr.l		%d0
	move.w		(%a0)+,%d0						// faulting opcode
	move.l		%a0,2(%sp)						// adjust return address to skip instruction
	cmp.w		#0xa000,%d0						// expected?
	jbne		mb_trap_fatal					// ... no
	jsr			mb_linea_struct_ptr				// ... yes, fetch return ptr
	move.l		%d0,%a0							// return in both D0/A0
	rte

	/*
	 * Trap1 handler. Complicated by Super shenanigans.
	 */
	.globl mb_trap1
	.globl mb_bdos_dispatch
	.type mb_trap1, @function
mb_trap1:
	btst.b  	#5,(%sp)						// called from supervisor mode?
	jbne		1f								// ... yes
	move.l		%usp,%a0						// args on user stack
	cmp.w		#32,(%a0)						// Super?
	jbeq		3f								// ... yes
	jbra		2f
1:
	lea			8(%sp),%a0						// args behind trap frame
	cmp.w		#32,(%a0)						// Super?
	jbeq		5f								// ... yes
2:
	pea			2(%a0)							// address of function args
	clr.l		%d0								// clear high word
	move.w		(%a0),%d0						// ... and get low word
	move.l		%d0,-(%sp)						// ... of function code
    jsr			mb_bdos_dispatch				// call dispatcher
    addq.l		#8,%sp							// adjust stack
    rte											// return already in D0
	.size mb_trap1, .-mb_trap1

3:												// Super from user mode
	ori         #0x0700,%sr						// mask interrupts while we shuffle stacks
	cmp.l		#1,(%a0)						// query?
	jbne		4f								// ...no
	clr.l		%d0								// SUP_USER
	rte
4:
	move.l		%sp,%a0							// save SSP to return
	move.l		%usp,%a1						// user stack
	sub.l		#8,%a1							// ... make space for frame
	move.w		(%a0),%d0						// copy SR
	or.w		#0x2000,%d0						// ... with S set
	move.w		%d0,(%a1)						// to new frame
	move.l		2(%a0),2(%a1)					// copy PC
	move.w		6(%a0),6(%a1)					// copy format / vector
	move.l		%a1,%sp							// switch stacks
	move.l		%a0,%d0							// return old SSP
	rte

5:												// Super from supervisor mode
	ori         #0x0700,%sr						// mask interrupts while we shuffle stacks
	cmp.l		#1,(%a0)						// query?
	jbne		6f								// ...no
	moveq.l		#1,%d0							// SUP_SUPER
	rte
6:
	move.l		(%a0),%a0						// previous SSP with original trapframe
	move.w		(%sp),%d0						// copy SR
	and.w		#0xdfff,%d0						// ... with S clear
	move.w		%d0,(%a0)						// back to old frame
	move.l		2(%sp),2(%a0)					// copy return address
	move.l		%a0,%sp							// swap to previous SSP
	rte

	/*
	 * Trap handler for simple traps not requiring special handling.
	 */
	.macro MB_SIMPLE_TRAP name, handler
	.globl \name
	.globl \handler
	.type \name, @function
\name:
	btst.b  	#5,(%sp)						// called from supervisor mode?
	jbne		1f								// ... yes
	move.l		%usp,%a0						// args on user stack
	jbra		2f
1:
	lea			8(%sp),%a0						// args behind trap frame
2:
	pea			2(%a0)							// address of function args
	clr.l		%d0								// clear high word
	move.w		(%a0),%d0						// ... and get low word
	move.l		%d0,-(%sp)						// ... of function code
    jsr			\handler						// call dispatcher
    addq.l		#8,%sp							// adjust stack
    rte											// return already in D0
	.size \name, .-\name
	.endm

	MB_SIMPLE_TRAP mb_trap2, mb_vdi_dispatch
	MB_SIMPLE_TRAP mb_trap13, mb_bios_dispatch
	MB_SIMPLE_TRAP mb_trap14, mb_xbios_dispatch

	/*
	 * Default exception handler.
	 */
	.globl mb_trap_fatal
	.globl mb_trap_panic
	.type mb_trap_fatal, @function
mb_trap_fatal:
    ori         #0x0700,%sr                     // disable interrupts
                                                // 040/060: guaranteed to be executed
                                                // before another interrupt will be taken.
                                                // 030: implied by similar wording to 040
                                                // others: ???
    movem.l     %d0-%d7/%a0-%a7, -(%sp)         // stack GP regs
    jsr			mb_trap_panic
    bra			.
	.size mb_trap_fatal, .-mb_trap_fatal


	.section .note.GNU-stack,"",@progbits
