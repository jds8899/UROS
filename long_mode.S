/*
** SCCS ID:	%W%	%G%
**
** File:	?
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	?
*/

#include "bootstrap.h"

	.code32

  .globl _long_mode_init

_long_mode_init:
	cli
	call	check_cpuid
	call	check_long_mode

	call	enable_paging
	call	gdt_setup
	ret

check_cpuid:
	pushfl
	pop		%eax
	mov		%eax, %ecx
	//mov		$1, %ebx
	//shl		$21, %ebx
	//xor		%ebx, %eax
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
	//mov		$1, %ebx
	//shl		$29, %ebx
	//test	%edx, %ebx
	test	$1 << 29, %edx
	jz		no_long_mode
	ret


enable_paging:
	mov		$RMTEXT_ADDRESS, %edi
	mov		%edi, %cr3
	xor		%eax, %eax
	mov		$4096, %ecx
	rep		stosl
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
	//mov		$1, %edx
	//shl		$5, %edx
	//or		%edx, %eax
	or		$1 << 5, %eax
	mov		%eax, %cr4

	mov		$0xC0000080, %ecx
	rdmsr
	//mov		$1, %edx
	//shl		$8, %edx
	//or		%edx, %eax
	or		$1 << 8, %eax
	wrmsr

	mov		%cr0, %eax
	//mov		$1, %edx
	//shl		$31, %edx
	//or		%edx, %eax
	or		$1 << 31, %eax
	mov		%eax, %cr0

	ret

no_long_mode:
	hlt

gdt_setup:
	call	move_gdt
	lgdt	gdt_48

	.byte	0xEA
	.long	long_mode_entry
	.word	8

	.code64
long_mode_entry:
	mov		$0x7000, %rbx
	movq	$0x1F201F201F201F20, %rax
	movq	%rax, (%rbx)
	hlt

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
	xor		%eax, %eax
	mov		%eax, %es
	mov		$start_gdt, %esi
	mov		$GDT_ADDRESS, %edi
	movl	$gdt_len, %ecx
	cld
	rep	movsb
	ret

gdt_48:
	.word	0x2000		/* 1024 GDT entries x 8 bytes/entry = 8192 */
	.quad	GDT_ADDRESS

/*
** The GDT.  This cannot be created in C because the bootstrap is not
** linked with that code.
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
	.byte	0x9A	/* 1 00 1 1010: present, prio 0, C/D, R/E code */
	.byte	0xAF	/* 1 1 00 1111: 4K, 32-bit, 0, 0, limit[19:16] */
	.byte	0x00

data_seg:	/* limit FFFFF, base 0, R/W data seg, 32-bit 4K */
	.word	0x0000
	.word	0x0000
	.byte	0x00
	.byte	0x92	/* 1 00 1 0010: present, prio 0, C/D, R/W data */
	.byte	0x00
	.byte	0x00

end_gdt:
gdt_len = end_gdt - start_gdt

