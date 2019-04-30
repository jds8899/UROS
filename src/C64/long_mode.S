/*
** SCCS ID:	%W%	%G%
**
** File:	long_mode.S
**
** Author:	Jonathan Schenk
**
** Contributor:
**
** Description:	Transitions CPU from Protected Mode to Long Mode
*/

#include "bootstrap.h"

	.code32

	.globl _long_mode_init

/*
** Entry point for Long Mode initialization.
*/
_long_mode_init:
	cli
	call	check_cpuid
	call	check_long_mode

	call	enable_paging
	//call	xmm_enable
	call	gdt_setup
	ret

/*
** Checks if CPUID instruction is available.
*/
check_cpuid:
	pushfl
	pop	%eax
	mov	%eax, %ecx
	xor	$1 << 21, %eax
	push	%eax
	popfl

	pushfl
	pop	%eax
	push	%ecx
	popfl

	xor	%ecx, %eax
	jz	no_cpuid
	ret

/*
** Realistically this should just bump back to a 32 bit OS
*/
no_cpuid:
	hlt

/*
** Does the CPU support Long Mode?
*/
check_long_mode:
	mov	$0x80000000, %eax
	cpuid
	cmp	$0x80000001, %eax
	jb	no_long_mode

	mov	$0x80000001, %eax
	cpuid
	test	$1 << 29, %edx
	jz	no_long_mode
	ret

/*
** Enables floating point stuff (unused)
*/
xmm_enable:
	mov	%cr0, %eax
	and	$0xFFFB, %ax
	or	$0x2, %ax
	mov	%eax, %cr0
	mov	%cr4, %eax
	or	$3 << 9, %ax
	mov	%eax, %cr4
	ret

/*
** Sets up 4 level page table and maps 2MB
*/
enable_paging:
	mov	$RMTEXT_ADDRESS, %edi
	mov	%edi, %cr3
	mov	%cr3, %edi

	mov	$RMTEXT_ADDRESS, %edx
	add	$0x1003, %edx
	movl	%edx, (%edi)
	add	$0x1000, %edi
	add	$0x1000, %edx
	movl	%edx, (%edi)
	add	$0x1000, %edi
	add	$0x1000, %edx
	movl	%edx, (%edi)
	add	$0x1000, %edi

	mov	$0x00000003, %ebx
	mov	$512, %ecx

set_entry:
	movl	%ebx, (%edi)
	add	$0x1000, %ebx
	add	$8, %edi
	loop	set_entry

	mov	%cr4, %eax
	or	$1 << 5, %eax
	mov	%eax, %cr4

	mov	$0xC0000080, %ecx
	rdmsr
	or	$1 << 8, %eax
	wrmsr

	mov	%cr0, %eax
	or	$1 << 31, %eax
	mov	%eax, %cr0

	ret

/*
** Again, this should just bump back to a 32 bit OS
*/
no_long_mode:
	hlt

/*
** Sets up 64 bit GDT
*/
gdt_setup:
	call	move_gdt
	lgdt	gdt_48

	// Hard coded jump to Long Mode that sets cs
	.byte	0xEA
	.long	long_mode_entry
	.word	GDT64_CODE

	.code64

/*
** Sets up 64 bit IDT (Apparently I never call this which means IDT 
** has 128 vecs)
*/
idt_setup:
	lidt	idt_48
	ret

/*
** We are now in Long Mode. Set up segment regs, initialize OS and 
** start the init process with a jump to __isr_restore.
*/
long_mode_entry:
	cli
	xor	%eax, %eax
	mov	$GDT64_DATA, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov	%ax, %gs
	xor	%ax, %ax
	mov	%ax, %ss
	movq	$TARGET_STACK, %rbp	/* set up the system frame pointer */
	movq	%rbp, %rsp		/* and stack pointer */

	.extern	rs_sys_init
	call	_km_init
	call	rs_sys_init

	jmp	__isr_restore   // defined in isr_stubs.S

	.code32

/*
** Move the GDT entries from where they are to location 0000:0000
**
** GDT values hard coded because I couldn't get the copy to work
** due to weirdness with segment registers.
*/
move_gdt:
	movl    $0x0000FFFF, %eax
	movl    %eax, GDT64_ADDRESS
	movl    $0x00010000, %eax
	movl    %eax, GDT64_ADDRESS + 0x4
	movl    $0x00000000, %eax
	movl    %eax, GDT64_ADDRESS + 0x8
	movl    $0x00209800, %eax
	movl    %eax, GDT64_ADDRESS + 0xc
	movl    $0x00000000, %eax
	movl    %eax, GDT64_ADDRESS + 0x10
	movl    $0x00009200, %eax
	movl    %eax, GDT64_ADDRESS + 0x14
	ret

gdt_48:
	.word	0x18
	.quad	GDT64_ADDRESS
idt_48:
	.word	0x1000		/* 256 interrupts */
	.quad	IDT_ADDRESS

/*
** The GDT. Not actually used because I couldn't get the copy to work.
** Left for someone more powerful than I to get it to work.
*/
start_gdt:

null_seg:	/* limit FFFFF, base 0, R/W data seg, 32-bit 4K */
	.word	0xFFFF	/* limit[15:0] */
	.word	0x0000	/* base[15:0] */
	.byte	0x00	/* base[23:16] */
	.byte	0x00	/* access byte */
	.byte	0x01	/* granularity */
	.byte	0x00	/* base[31:24] */

code_seg:	/* limit FFFFF, base 0, R/E code seg, 32-bit 4K */
	.word	0x0000
	.word	0x0000
	.byte	0x00
	.byte	0x98	
	.byte	0x20	
	.byte	0x00

data_seg:	/* limit FFFFF, base 0, R/W data seg, 32-bit 4K */
	.word	0x0000
	.word	0x0000
	.byte	0x00
	.byte	0x90	
	.byte	0x00
	.byte	0x00

end_gdt:
gdt_len = end_gdt - start_gdt

