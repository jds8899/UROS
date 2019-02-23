# 1 "long_mode.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "long_mode.S"
# 13 "long_mode.S"
# 1 "bootstrap.h" 1
# 14 "long_mode.S" 2

 .code32

  .globl _long_mode_init

_long_mode_init:
 cli
 call check_cpuid
 call check_long_mode

 call enable_paging
 call gdt_setup
 ret

check_cpuid:
 pushfl
 pop %eax
 mov %eax, %ecx



 xor $1 << 21, %eax
 push %eax
 popfl

 pushfl
 pop %eax
 push %ecx
 popfl

 xor %ecx, %eax
 jz no_cpuid
 ret

no_cpuid:
 hlt

check_long_mode:
 mov $0x80000000, %eax
 cpuid
 cmp $0x80000001, %eax
 jb no_long_mode

 mov $0x80000001, %eax
 cpuid



 test $1 << 29, %edx
 jz no_long_mode
 ret


enable_paging:
 mov $0x00003000, %edi
 mov %edi, %cr3
 xor %eax, %eax
 mov $4096, %ecx
 rep stosl
 mov %cr3, %edi

 mov $0x00003000, %edx
 add $0x1003, %edx
 movl %edx, (%edi)
 add $0x1000, %edi
 add $0x1000, %edx
 movl %edx, (%edi)
 add $0x1000, %edi
 add $0x1000, %edx
 movl %edx, (%edi)
 add $0x1000, %edi

 mov $0x00000003, %ebx
 mov $512, %ecx

set_entry:
 movl %ebx, (%edi)
 add $0x1000, %ebx
 add $8, %edi
 loop set_entry

 mov %cr4, %eax



 or $1 << 5, %eax
 mov %eax, %cr4

 mov $0xC0000080, %ecx
 rdmsr



 or $1 << 8, %eax
 wrmsr

 mov %cr0, %eax



 or $1 << 31, %eax
 mov %eax, %cr0

 ret

no_long_mode:
 hlt

gdt_setup:
 call move_gdt
 lgdt gdt_48

 .byte 0xEA
 .long long_mode_entry
 .word 8

 .code64
long_mode_entry:
 mov $0x7000, %rbx
 movq $0x1F201F201F201F20, %rax
 movq %rax, (%rbx)
 hlt

 .code32





kill:
 mov $0xC0000080, %ecx
 rdmsr
 mov $1, %edx
 shl $8, %edx
 or %edx, %eax
 wrmsr







move_gdt:
 xor %eax, %eax
 mov %eax, %es
 mov $start_gdt, %esi
 mov $0x00000500, %edi
 movl $gdt_len, %ecx
 cld
 rep movsb
 ret

gdt_48:
 .word 0x2000
 .quad 0x00000500





start_gdt:

 null_seg:
 .word 0xFFFF
 .word 0x0000
 .byte 0x00
 .byte 0x00
 .byte 0x01
 .byte 0x00

code_seg:
 .word 0x0000
 .word 0x0000
 .byte 0x00
 .byte 0x9A
 .byte 0xAF
 .byte 0x00

data_seg:
 .word 0x0000
 .word 0x0000
 .byte 0x00
 .byte 0x92
 .byte 0x00
 .byte 0x00

end_gdt:
gdt_len = end_gdt - start_gdt
