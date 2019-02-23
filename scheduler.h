/*
** SCCS ID:	@(#)scheduler.h	1.1	3/29/18
**
** File:	scheduler.h
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	Scheduler module declarations
*/

#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "common.h"

#include "pcbs.h"
#include "queues.h"

/*
** General (C and/or assembly) definitions
*/

// standard quantum for processes:  5 ticks

#define	QUANTUM_STD	5
#define	QUANTUM_SHORT	3

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

/*
** Types
*/

/*
** Globals
*/

extern queue_t _ready[N_PRIOS];		// the ready queue
extern pcb_t *_current;			// the current process

/*
** Prototypes
*/

/*
** _sched_init()
**
** initialize the scheduler module
*/
void _sched_init( void );

/*
** _schedule(pcb)
**
** schedule a process for execution
*/
void _schedule( pcb_t *pcb );

/*
** _dispatch()
**
** give the CPU to a process
*/
void _dispatch( void );

#endif

#endif
