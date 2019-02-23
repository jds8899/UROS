/*
** SCCS ID:	@(#)stacks.h	1.1	3/29/18
**
** File:	stacks.h
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	Declarations for the stack module
*/

#ifndef _STACKS_H_
#define _STACKS_H_

#include "common.h"

/*
** General (C and/or assembly) definitions
*/

// number of 32-bit unsigned integers in a stack

#define	STACK_SIZE_U32	1024

// total size of a stack

#define	STACK_SIZE	(STACK_SIZE_U32 * sizeof(uint32_t))

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

/*
** Types
*/

// stack structure

typedef uint32_t stack_t[STACK_SIZE_U32];

/*
** Globals
*/

extern stack_t _system_stack;		// the OS stack
extern uint32_t *_system_esp;		// the OS %ESP value

/*
** Prototypes
*/

/*
** _stk_init()
**
** initializes all stack-related data structures
*/
void _stk_init( void );

/*
** _stk_alloc()
**
** allocate a stack structure
**
** returns a pointer to the stack, or NULL on failure
*/
stack_t *_stk_alloc( void );

/*
** _stk_free(stk)
**
** deallocate a stack
*/
void _stk_free( stack_t *stk );

/*
** _stk_dump(msg,stk,limit)
**
** dump the contents of the provided stack, eliding
** duplicate lines
*/
void _stk_dump( const char *msg, stack_t *stack, uint32_t limit );

/*
** _stk_setup(stack,entry,argv,argc,len)
**
** set up 'stack' with a fresh user context
**
** returns a pointer to the context save area in the stack
*/
context_t *_stk_setup( stack_t *stack, uint32_t entry, char *argv[],
      int argc, int len );

#endif

#endif
