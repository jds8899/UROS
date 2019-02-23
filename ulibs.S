/*
** SCCS ID:	@(#)ulibs.S	1.1	3/29/18
**
** File:	ulibs.S
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	assembly-language user-level library functions
*/

#define	__SP_ASM__

#include "syscalls.h"

/*
** System call stubs
**
** All have the same structure:
**
**      move a code into EAX
**      generate the interrupt
**      return to the caller
**
** As these are simple "leaf" routines, we don't use
** the standard enter/leave method to set up a stack
** frame - that takes time, and we don't really need it.
*/

#define	SYSCALL(name) \
	.globl	name			; \
name:					; \
	movl	$SYS_##name, %eax	; \
	int	$INT_VEC_SYSCALL	; \
	ret

SYSCALL(exit)
SYSCALL(read)
SYSCALL(write)
SYSCALL(fork)
SYSCALL(exec)
SYSCALL(kill)
SYSCALL(wait)
SYSCALL(sleep)
SYSCALL(time)
SYSCALL(pid)
SYSCALL(ppid)
SYSCALL(get_prio)
SYSCALL(set_prio)
SYSCALL(dumpme)

/*
** This is a bogus system call; it's here so that we can test
** our handling of out-of-range syscall codes in the syscall ISR.
*/

SYSCALL(bogus)

/*
** do_exit()
**
** support function used as the "return to" address
** for user main() routines; it just calls exit(%eax)
*/

	.globl  do_exit
do_exit:
	pushl   %eax    // use whatever was in EAX as the status
	call    exit    // terminate this process

/*
** get_ra()
**
** Get the return address for the calling function
** (i.e., where whoever called us will go back to)
*/
	.globl	get_ra

get_ra:
	// DO NOT create a stack frame - use the caller's
	//
	// Caller's return address is between the saved EBP
	// and its first parameter
	movl	4(%ebp), %eax
	ret
