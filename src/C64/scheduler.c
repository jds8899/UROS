/*
** SCCS ID:	@(#)scheduler.c	1.1	3/29/18
**
** File:	scheduler.c
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	Scheduler module implementations
*/

#define	__SP_KERNEL__

#include "common.h"

#include "scheduler.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

/*
** PUBLIC GLOBAL VARIABLES
*/

queue_t _ready[N_PRIOS];	// the ready queue
pcb_t *_current;		// the current process

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/*
** _sched_init()
**
** initialize the scheduler module
*/
void _sched_init( void ) {
   int i;
   
   // report that we're here

   c_puts( " SCHED" );

   // iterate through all the queue levels

   for( i = 0; i < N_PRIOS; ++i ) {

      // allocate this level's queue
      _ready[i] = _q_alloc( NULL );

      // if that failed, we're in deep trouble
      if( _ready[i] == NULL ) {
         c_printf( "ERR: sched init alloc %d failed\n", i );
         _kpanic( "_sched_init", "ready queue alloc" );
      }
   }
}

/*
** _schedule(pcb)
**
** schedule a process for execution
*/
void _schedule( pcb_t *pcb ) {

   // if no PCB, nothing to do
   if( pcb == NULL ) {
      return;
   }
   
   // verify that the priority level makes sense

   if( pcb->prio >= N_PRIOS ) {
      
      // oops - it doesn't.  this should never happen!
      c_printf( "ERR: sched pid %d prio %d\n",
         pcb->pid, pcb->prio );
      _kpanic( "_schedule", "bad process priority value" );
   }
   
   // all's well; just add it to the indicated queue

   pcb->state = ST_READY;
   _q_insert( _ready[pcb->prio], pcb );
}


/*
** _dispatch()
**
** give the CPU to a process
*/
void _dispatch( void ) {
   pcb_t *pcb = NULL;
   int i;

#ifdef INCLUDE_SHELL
   // check to see if there is any console input

   if( c_input_queue() > 0 ) {
      // yes - deal with whatever is there
      _shell();
   }
#endif
   
   // iterate through the levels until we find one
   // that has a process.  dispatch it.
   for( i = 0; i < N_PRIOS; ++i ) {
      if( _q_size(_ready[i]) > 0 ) {
         pcb = _q_remove( _ready[i] );
         break;
      }
   }

   /*
   ** when we leave the loop, we should have our lucky winner.
   ** if 'pcb' is still NULL, though, there were no processes
   ** anywhere.  this should never happen, as there should
   ** always be an "idle" process hanging out in the bottom-most
   ** queue.
   */
   
   if( pcb == NULL ) {
      _kpanic( "_dispatch", "no ready process" );
   }
   
   // found one - it's now the current process

   pcb->state = ST_RUNNING;
   pcb->ticks = QUANTUM_STD;
   _current = pcb;

#if 0
   if( _current->pid > PID_FIRST_USER ) { // skip init() and idle()
      _active_dump( "Process table", TRUE );
   }
#endif
}
