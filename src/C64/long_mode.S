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

_long_mode_init:
	cli
	call	check_cpuid
	call	check_long_mode

	call	enable_paging
	//call	xmm_enable
	call	gdt_setup
	ret

check_cpuid:
	pushfl
	pop		%eax
	mov		%eax, %ecx
	xor		$1 << 21, %eax
	push	%eax
	popfl

	pushfl
	pop		%eax
	push	%ecx
	popfl

	xor		%ecx, %eax
	jz		no_cpuid
	ret

no_cpuid:
	hlt

check_long_mode:
	mov		$0x80000000, %eax
	cpuid
	cmp		$0x80000001, %eax
	jb		no_long_mode

	mov		$0x80000001, %eax
	cpuid
	test	$1 << 29, %edx
	jz		no_long_mode
	ret

xmm_enable:
	mov		%cr0, %eax
	and		$0xFFFB, %ax
	or		$0x2, %ax
	mov		%eax, %cr0
	mov		%cr4, %eax
	or		$3 << 9, %ax
	mov		%eax, %cr4
	ret

enable_paging:
	mov		$RMTEXT_ADDRESS, %edi
	mov		%edi, %cr3
	mov		%cr3, %edi

	mov		$RMTEXT_ADDRESS, %edx
	add		$0x1003, %edx
	movl	%edx, (%edi)
	add		$0x1000, %edi
	add		$0x1000, %edx
	movl	%edx, (%edi)
	add		$0x1000, %edi
	add		$0x1000, %edx
	movl	%edx, (%edi)
	add		$0x1000, %edi

	mov		$0x00000003, %ebx
	mov		$512, %ecx

set_entry:
	movl	%ebx, (%edi)
	add		$0x1000, %ebx
	add		$8, %edi
	loop	set_entry

	mov		%cr4, %eax
	or		$1 << 5, %eax
	mov		%eax, %cr4

	mov		$0xC0000080, %ecx
	rdmsr
	or		$1 << 8, %eax
	wrmsr

	mov		%cr0, %eax
	or		$1 << 31, %eax
	mov		%eax, %cr0

	ret

no_long_mode:
	hlt

gdt_setup:
	call	move_gdt
	lgdt	gdt_48
	//jmp		*lme

	.byte	0xEA
	.long	long_mode_entry
	.word	GDT64_CODE

	.code64

idt_setup:
	lidt	idt_48
	ret

long_mode_entry:
	cli
	xor		%eax, %eax
	mov		$GDT64_DATA, %ax
	mov		%ax, %ds
	mov		%ax, %es
	mov		%ax, %fs
	mov		%ax, %gs
	xor		%ax, %ax
	mov		%ax, %ss
	//movq	$0x2f592f412f4b2f4f, %rax
	//movq	%rax, (0xb8000)
	movq	$TARGET_STACK, %rbp	/* set up the system frame pointer */
	movq	%rbp, %rsp	/* and stack pointer */

	.extern	rs_sys_init
	call	rs_sys_init
	hlt

	call	_init
	jmp	__isr_restore   // defined in isr_stubs.S

	.code32
/*
** This bad boy causes a fault and makes the computer restart.
** Good for when you don't have any output and want to see how
** far your code is getting.
*/
kill:
	mov		$0xC0000080, %ecx
	rdmsr
	mov		$1, %edx
	shl		$8, %edx
	or		%edx, %eax
	wrmsr

/*
** Move the GDT entries from where they are to location 0000:0000
**
** As with the IDTR and GDTR loads, we need the offset for the GDT
** data from the beginning of the segment (0000:0000).
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
** The GDT
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
	.byte	0x98	/* : present, prio 0, C/D, R/E code */
	.byte	0x20	/* : 4K, 32-bit, 0, 0, limit[19:16] */
	.byte	0x00

data_seg:	/* limit FFFFF, base 0, R/W data seg, 32-bit 4K */
	.word	0x0000
	.word	0x0000
	.byte	0x00
	.byte	0x90	/* : present, prio 0, C/D, R/W data */
	.byte	0x00
	.byte	0x00

end_gdt:
gdt_len = end_gdt - start_gdt

