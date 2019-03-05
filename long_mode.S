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
	//xor		%ax, %ax		// Make sure es and ds are 0
	//mov		%eax, %ds		// For later copy operations
	//mov		%eax, %es
	call	check_cpuid
	call	check_long_mode

	call	enable_paging
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


enable_paging:
	mov		$RMTEXT_ADDRESS, %edi
	mov		%edi, %cr3
	//xor		%eax, %eax
	//mov		$4096, %ecx
	//rep		stosl
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
	movq	$0x2f592f412f4b2f4f, %rax
	movq	%rax, (0xb8000)
	call	c_io_init
	//call	c_clearscreen
	//mov		$0x700000000, %rbx
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
	//mov		$start_gdt, %esi
	//mov		$GDT64_ADDRESS, %edi
	//movl	$gdt_len, %ecx
	//cld
	//rep	movsb
	movl    $0x00010000, %eax
	movl    %eax, 0x1504
	movl    $0x0000FFFF, %eax
	movl    %eax, 0x1500
	movl    $0x00209800, %eax
	movl    %eax, 0x150c
	movl    $0x00000000, %eax
	movl    %eax, 0x1508
	movl    $0x00009000, %eax
	movl    %eax, 0x1514
	movl    $0x00000000, %eax
	movl    %eax, 0x1510
	ret

gdt_48:
	.word	0x18
	.quad	GDT64_ADDRESS

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

