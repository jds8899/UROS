/*
** SCCS ID:	@(#)syscalls.h	1.1	3/29/18
**
** File:	syscalls.h
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	System call module declarations
*/

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "common.h"

/*
** General (C and/or assembly) definitions
*/

// system call codes
//
// these are used in the user-level C library stub functions

#define	SYS_exit	0
#define	SYS_read	1
#define	SYS_write	2
#define	SYS_fork	3
#define	SYS_exec	4
#define	SYS_kill	5
#define	SYS_wait	6
#define	SYS_sleep	7
#define	SYS_time	8
#define	SYS_pid		9
#define	SYS_ppid	10
#define	SYS_get_prio	11
#define	SYS_set_prio	12
#define	SYS_dumpme	13

// UPDATE THIS DEFINITION IF MORE SYSCALLS ARE ADDED!
#define	N_SYSCALLS	14

// dummy system call code to test our ISR

#define	SYS_bogus	(N_SYSCALLS + 100)

// interrupt vector entry for system calls

#define	INT_VEC_SYSCALL		0x42

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

#ifdef	__SP_KERNEL__

// the following declarations should only be seen by the kernel

#include "queues.h"

/*
** Types
*/

/*
** Globals
*/

/*
** Prototypes
*/

/*
** _sys_init()
**
** initializes all syscall-related data structures
*/
void _sys_init( void );

#endif

#endif

#endif
