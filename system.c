/*
** SCCS ID:	@(#)system.c	1.1	3/29/18
**
** File:	system.c
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	miscellaneous OS support functions
*/

#define	__SP_KERNEL__

#include "common.h"

#include "common.h"

#include "system.h"
#include "queues.h"
#include "clock.h"
#include "pcbs.h"
#include "bootstrap.h"
#include "syscalls.h"
#include "sio.h"
#include "scheduler.h"

// need init() address
#include "users.h"

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

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/*
** _bite(pcb)
**
** turn a process into a zombie, or give its status to
** a waiting parent
*/
void _bite( pcb_t *pcb ) {

#if defined(SANITY_CHECK)
   if( pcb == NULL ) {
      _kpanic( "_bite", "NULL pcb parameter" );
   }
#endif

   // reparent any children this process has

   if( pcb->children > 0 ) {

      // if init isn't there, we're in deep trouble
      if( _init_pcb == NULL ) {
         _kpanic( "_bite", "init is gone?!?!?" );
      }

      // locate the first child
      pcb_t *whom = _pcb_find_parent( pcb->pid );

      // as long as we still have one to fix,
      while( whom != NULL ) {
         // reparent it
         whom->ppid = PID_INIT;
         pcb->children -= 1;
         _init_pcb->children += 1;
         whom = _pcb_find_parent( pcb->pid );
      }

      // did we get them all?
      if( pcb->children != 0 ) {
         c_printf( "ERR: pid %d has %d kids, none found!\n",
            pcb->pid, pcb->children );
         _kpanic( "_bite", "kid(s) don't exist???" );
      }
   }
   
   // find the parent of this process

   pcb_t *parent = _pcb_find( pcb->ppid );
   if( parent == NULL ) {
      c_printf( "ERR: pid %d: ppid %d doesn't exist\n",
           pcb->pid, pcb->ppid );
      _kpanic( "_bite", "no parent for process!" );
   }
   
   /*
   ** ok, we found the parent.  if the parent it currently
   ** wait()ing for a chile, unblock it and give it the
   ** info on this process; otherwise, put this process
   ** onto the zombie queue
   */

   if( parent->state == ST_WAITING ) {

      // pull the parent from the wait queue
      pcb_t *victim = _q_remove_by( _waiting, (void *)(uint64_t)(parent->pid),
                           _q_find_pid );

      // make sure we got it!
      if( victim == NULL ) {
         c_printf(
            "ERR: pid %d: waiting parent %d not on queue!\n",
            pcb->pid, parent->pid );
         _kpanic( "_bite", "waiting parent not on queue" );
      } else if( victim != parent ) {
         c_printf(
            "ERR: pid %d: parent %d @ 0x%x != %d @ 0x%x\n",
            pcb->pid, parent->pid, (uint64_t) parent,
            victim->pid, (uint64_t) victim );
         _kpanic( "_bite", "found wrong waiting parent" );
      }

      // found it; give the parent the child's info

      RET(parent) = pcb->pid;
      int32_t *status = (int32_t *) ARG(parent,1);
      *status = pcb->exitstatus;

      // clean up the child
      _pcb_cleanup( pcb );

      // put the parent back in the ready queue
      _schedule( parent );

   } else {

      // add the child to the zombie horde
      _q_insert( _zombie, pcb );
   }
   
   // do NOT want to dispatch here!
   // whoever called us must do this!
}

/*
** _prune() - release any "spare" allocated memory
**
** Check with all the modules which may retain allocated memory
** for later re-use, and have them release it all to increase the
** pool of available storage.
**
** Returns the number of bytes released.
*/
uint32_t _prune( void ) {

   return( _q_prune() +
           _pcb_prune()
      // add any others here, summing their return values
   );
}

/*
** _init - system initialization routine
**
** Called by the startup code immediately before returning into the
** first user process.
*/
void _init( void ) {
   pcb_t *pcb;

   /*
   ** BOILERPLATE CODE - taken from basic framework
   **
   ** Initialize interrupt stuff.
   */

   __init_interrupts();   // IDT and PIC initialization

   /*
   ** Console I/O system.
   */

   c_io_init();   // also installs a console receiver interrupt handler
   for(;;){}
#ifdef ISR_DEBUGGING_CODE
   c_setscroll( 0, 7, 99, 99 );
   c_puts_at( 0, 6, "================================================================================" );
#endif

   /*
   ** 20175-SPECIFIC CODE STARTS HERE
   */

   /*
   ** Initialize various OS modules
   **
   ** Other modules (clock, SIO, syscall, etc.) are expected to
   ** install their own ISRs in their initialization routines.
   */

   c_puts( "System initialization starting\n" );
   c_puts( "------------------------------\n" );

   c_puts( "Modules: " );

   _km_init();       // kernel memory system (must be first)

   _q_init();        // queues (must be second)

   _clk_init();      // clock
   _pcb_init();      // processes
   _sched_init();    // scheduler
   _sio_init();      // serial i/o
   _stk_init();      // stacks

   _sys_init();      // syscalls (must be last, for _all_queues setup)

   c_puts( "\nModule initialization complete\n" );
   c_puts( "------------------------------\n" );
   __delay( 200 );  // about 5 seconds

   /*
   ** Create the initial process
   **
   ** Code largely stolen from _sys_fork() and _sys_exec(); if
   ** either or both of those routines change, SO MUST THIS!!!
   */

   // allocate a PCB and stack

   pcb = _pcb_alloc();
   if( pcb == NULL ) {
      _kpanic( "_init", "init() pcb alloc failed" );
   }

   pcb->stack = _stk_alloc();
   if( pcb->stack == NULL ) {
      _kpanic( "_init", "init() stack alloc failed" );
   }

   // initialize the stack with the standard context

   char *argv[] = { "init", NULL };
   pcb->context = _stk_setup( pcb->stack, (uint64_t) init, argv, 1, 5 );
   if( pcb->context == NULL ) {
      _kpanic( "_init", "init() stack setup failed" );
   }

   // _pcb_dump( "init()", pcb );
   // _stk_dump( "init() stack", pcb->stack, 8 );
   // __delay( 2000 );  // about 50 seconds

   // set up various PCB fields
   pcb->pid = pcb->ppid = PID_INIT;
   pcb->prio = PRIO_SYS_HIGH;
   pcb->children = 0;

   // remember it
   _init_pcb = pcb;

   // put it on the ready queue
   _schedule( pcb );

   // this is our first active process

   _active[0] = pcb;
   _num_active = 1;

   /*
   ** Create the idle() process
   **
   ** Philosophical question: should we create it here, or
   ** should we let init() create it?
   **
   ** We opt for the latter, as it means less work for us. :-)
   */

   /*
   ** Turn on the SIO receiver (the transmitter will be turned
   ** on/off as characters are being sent)
   */

   _sio_enable( SIO_RX );

   // dispatch the first user process

   _dispatch();

   /*
   ** END OF 20175-SPECIFIC CODE
   **
   ** Finally, report that we're all done.
   */

   c_puts( "System initialization complete.\n" );
   c_puts( "-------------------------------\n" );
}

#ifdef INCLUDE_SHELL
/*
** _shell - extremely simple shell for handling console input
**
** Called whenever we want to take input from the console and
** act upon it (e.g., for debugging the kernel)
*/
void _shell( void ) {
   int ch;

   c_puts( "\nInteractive mode\n? " );
   ch = c_getchar();

   // loop until we get an "exit" indicator

   while( 1 ) {

// copied from c_io.c
#define EOT	'\04'
      if( ch == 'x' || ch == EOT ) {
         break;
      }

      // c_printf( "%c\n", ch );

      switch( ch ) {

	 case 'h':  // help message
	    c_puts( "\nCommands:\n" );
	    c_puts( "   q  -- dump the queues\n" );
	    c_puts( "   a  -- dump the active table\n" );
	    c_puts( "   p  -- dump the active table and all PCBs\n" );
	    c_puts( "   s  -- dump stacks for active processes\n" );
	    break;

         case 'q':  // dump the queues
            _q_dump_all( "\nQueue contents\n" );
	    break;

	 case 'a':  // dump the active table
	    _active_dump( "\nActive processes", FALSE );
	    break;

	 case 'p':  // dump the active table and all PCBs
	    _active_dump( "\nActive processes", TRUE );
	    break;

	 case 's':  // dump stack info for all active PCBS
	    c_puts( "\nActive stacks (w/5-sec. delays):\n" );
	    for( int i = 0; i < MAX_PROCS; ++i ) {
	       if( _active[i] != NULL ) {
	          pcb_t *pcb = _active[i];
		  c_printf( "pid %5d: ", pcb->pid );
		  c_printf( "EIP %08x, ", pcb->context->rip );
		  _stk_dump( NULL, pcb->stack, 12 );
		  __delay( 200 );
		}
            }
	    break;
	 
	 default:
	    c_printf( "shell: unknown request '%c'\n", ch );
      }

      c_puts( "\n? " );
      ch = c_getchar();

   }

   c_puts( "\nExiting interactive mode\n" );

}
#endif
