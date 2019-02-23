/*
** SCCS ID:	@(#)pcbs.h	1.1	3/29/18
**
** File:	pcbs.h
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	PCB-related declarations
*/

#ifndef _PCBS_H_
#define _PCBS_H_

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

// PID of the initial process

#define	PID_INIT	1

// first "user" PID

#define	PID_FIRST_USER	100

// REG(pcb,x) -- access a specific register in a process context

#define	REG(pcb,x)	((pcb)->context->x)

// RET(pcb) -- access return value register in a process context

#define	RET(pcb)	((pcb)->context->eax)

// ARG(pcb,n) -- access argument #n from the indicated process
//
// ARG(pcb,0) --> return address
// ARG(pcb,1) --> first parameter
// ARG(pcb,2) --> second parameter
// etc.
//
// IF THE PARAMETER PASSING MECHANISM CHANGES, SO MUST THIS

#define	ARG(pcb,n)	( ( (uint32_t *) (((pcb)->context) + 1) ) [(n)] )

/*
** Types
*/

/*
** process states
**
** ordering is critical here - NEW must be the
** second entry, and READY must be the last entry
*/

enum e_states {
   ST_UNUSED = 0,
   ST_NEW = 1,
   ST_RUNNING = 2,
   ST_SLEEPING = 3,
   ST_WAITING = 4,
   ST_BLOCKED_IO = 5,
   ST_KILLED = 6,
   ST_ZOMBIE = 7,
   ST_READY  // must always be last!
};

/*
** these definitions should not need to be changed
**
** FIRST_STATE and LAST_STATE are the first and last states
** which a process can actually be in
*/

#define FIRST_STATE	ST_NEW
#define LAST_STATE	ST_READY
#define N_STATES	(LAST_STATE + 1)

#define	VALID_STATE(n)	((n) >= ST_UNUSED && (n) <= LAST_STATE)

///
// process context structure
//
// NOTE:  the order of data members here depends on the
// register save code in isr_stubs.S!!!!
///

typedef struct context {
   uint32_t ss;
   uint32_t gs;
   uint32_t fs;
   uint32_t es;
   uint32_t ds;
   uint32_t edi;
   uint32_t esi;
   uint32_t ebp;
   uint32_t esp;
   uint32_t ebx;
   uint32_t edx;
   uint32_t ecx;
   uint32_t eax;
   uint32_t vector;
   uint32_t code;
   uint32_t eip;
   uint32_t cs;
   uint32_t eflags;
} context_t;

// include these here after the context definition

#include "stacks.h"
#include "queues.h"

///
// process control block
//
// members are ordered by size
///

typedef struct st_pcb {
   // 32-bit items
   context_t *context;		// context save area pointer
   stack_t *stack;		// per-process runtime stack
   uint32_t event;		// event indicator for waiting processes
   int32_t exitstatus;		// exit status for zombies

   // 16-bit items
   uint16_t pid;		// unique process id
   uint16_t ppid;		// PID of parent
   uint16_t children;		// count of currently-active children

   // 8-bit items
   uint8_t state;		// current process state
   uint8_t prio;		// scheduling priority
   uint8_t ticks;		// remaining execution quantum
} pcb_t;

/*
** Globals
*/

extern uint16_t _next_pid;	// next available PID

extern queue_t _free_pcbs;	// queue of unused PCBs

extern pcb_t *_active[MAX_PROCS];	// pointers to all active PCBs
extern uint32_t _num_active;		// count of active PCBs

extern pcb_t *_init_pcb;	// PCB of the initial process

/*
** Prototypes
*/

/*
** _pcb_init()
**
** initializes all PCB-related data structures
*/
void _pcb_init( void );

/*
** _pcb_prune()
**
** release any allocated space we aren't using
*/
uint32_t _pcb_prune( void );

/*
** _pcb_alloc()
**
** allocate a PCB structure
**
** returns a pointer to the PCB, or NULL on failure
*/
pcb_t *_pcb_alloc( void );

/*
** _pcb_free(pcb)
**
** deallocate a PCB
*/
void _pcb_free( pcb_t *pcb );

/*
** _pcb_find(whom)
**
** locate the PCB having the specified PID
**
** returns a pointer to the PCB, or NULL if not found
*/
pcb_t *_pcb_find( uint16_t whom );

/*
** _pcb_find_parent(whom)
**
** locate the PCB having the specified PPID
**
** returns a pointer to the PCB, or NULL if not found
*/
pcb_t *_pcb_find_parent( uint16_t whom );

/*
** _pcb_cleanup(pcb)
**
** clean up and free a pcb and its associated stack
*/
void _pcb_cleanup( pcb_t *pcb );

/*
** _pcb_dump(msg,pcb)
**
** dump the contents of this PCB to the console
*/
void _pcb_dump( const char *msg, pcb_t *pcb );

/*
** _context_dump(msg,context)
**
** dump the contents of this process context to the console
*/
void _context_dump( const char *msg, context_t *context );

/*
** _active_dump(msg,all)
**
** dump the contents of the "active processes" table
*/
void _active_dump( const char *msg, int all );

#endif

#endif
