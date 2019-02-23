/*
** SCCS ID:	@(#)pcbs.c	1.1	3/29/18
**
** File:	pcbs.c
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	PCB-related implementations
*/

#define	__SP_KERNEL__

#include "common.h"

#include "pcbs.h"

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

uint16_t _next_pid;	// next available PID

// we'll keep a queue of available PCBs to save some allocation time
queue_t _free_pcbs;	// queue of unused PCBs

pcb_t *_active[MAX_PROCS];	// pointers to all active PCBs
uint32_t _num_active;		// count of active PCBs

// the PCB of the initial process (to save some time elsewhere)

pcb_t *_init_pcb;

// table of state names

char _states[N_STATES][4] = {
   "UNU", "NEW", "RUN", "SLP", "WTG", "BIO", "KIL", "ZOM", "RDY"
};

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/*
** _pcb_init()
**
** initializes all PCB-related data structures
*/
void _pcb_init( void ) {

   // announce our presence
   c_puts( " PCB" );

   // start PIDs out at a known value
   _next_pid = PID_FIRST_USER;

   // allocate the free PCB queue
   _free_pcbs = _q_alloc( NULL );
   if( _free_pcbs == NULL ) {
      _kpanic( "_pcb_init", "can't alloc PCB queue" );
   }

   // initialize the process table
   for( int i = 0; i < MAX_PROCS; ++i ) {
      _active[i] = NULL;
   }

   _num_active = 0;

}

/*
** _pcb_prune()
**
** release any allocated space we aren't using
*/
uint32_t _pcb_prune( void ) {
   uint32_t sum = 0;

   while( _q_size(_free_pcbs) > 0 ) {
      ++sum;
      pcb_t *tmp = _q_remove( _free_pcbs );
      _kfree( tmp );
   }

   // return a byte count

   return( sum * sizeof(pcb_t) );
}

/*
** _pcb_alloc()
**
** allocate a PCB structure
**
** returns a pointer to the PCB, or NULL on failure
*/
pcb_t *_pcb_alloc( void ) {
   pcb_t *new;

   // first, check to see if we can re-use an old one

   if( _q_size(_free_pcbs) > 0 ) {
      new = _q_remove( _free_pcbs );
   } else {
     try_again:
      new = _kmalloc( sizeof(pcb_t) );
   }

   if( new == NULL ) {
      if( !_prune() ) {
         _kpanic( "_pcb_alloc", "alloc failed" );
      }
      goto try_again;
   }

   // clear out the space

   _kmemclr( (uint8_t *) new, sizeof(pcb_t) );

   // mark it as "in use"

   new->state = ST_NEW;

   return( new );
}

/*
** _pcb_free(pcb)
**
** deallocate a PCB
*/
void _pcb_free( pcb_t *pcb ) {

#if defined(SANITY_CHECK)
   if( pcb == NULL ) {
      _kpanic( "_pcb_free", "NULL argument" );
   }
#endif

   // mark it as currently available
   pcb->state = ST_UNUSED;

   // ensure it can't be found by a PID search
   pcb->pid = -1;
   pcb->ppid = -1;

   // save it for later re-use
   _q_insert( _free_pcbs, (void *) pcb );
}

/*
** _pcb_find(pid)
**
** locate the PCB having the specified PID
**
** returns a pointer to the PCB, or NULL if not found
*/
pcb_t *_pcb_find( uint16_t whom ) {

   for( int i = 0; i < MAX_PROCS; ++i ) {
      if( _active[i] && _active[i]->pid == whom ) {
         return( _active[i] );
      }
   }

   return( NULL );
}

/*
** _pcb_find_parent(pid)
**
** locate the PCB having the specified PPID
**
** returns a pointer to the PCB, or NULL if not found
*/
pcb_t *_pcb_find_parent( uint16_t whom ) {

   for( int i = 0; i < MAX_PROCS; ++i ) {
      if( _active[i] && _active[i]->ppid == whom ) {
         return( _active[i] );
      }
   }

   return( NULL );
}

/*
** _pcb_cleanup(pcb)
**
** clean up and free a pcb and its associated stack
*/
void _pcb_cleanup( pcb_t *pcb ) {

   // start by freeing the stack

   _stk_free( pcb->stack );

   // next, remove it from the "active processes" table

   for( int i = 0; i < MAX_PROCS; ++i ) {
      if( _active[i] == pcb ) {
         _active[i] = NULL;
	 break;
      }
   }

   // one fewer active process

   _num_active -= 1;

   // release the PCB into the wild

   _pcb_free( pcb );
}

/*
** _pcb_dump(msg,pcb)
**
** dump the contents of this PCB to the console
*/
void _pcb_dump( const char *msg, pcb_t *pcb ) {

   c_printf( "%s @ %08x: ", msg, (uint32_t) pcb );
   if( pcb == NULL ) {
      c_puts( " NULL???\n" );
      return;
   }

   c_printf( " pids %d/%d state ", pcb->pid, pcb->ppid );
   if( VALID_STATE(pcb->state) ) {
      c_puts( _states[pcb->state] );
   } else {
      c_printf( "? (%d)", pcb->state );
   }

   c_printf( "\n event %u xit %d", pcb->event, pcb->exitstatus );

   c_printf( " kids %d prio %d ticks %d",
             pcb->children, pcb->prio, pcb->ticks );

   c_printf( " context %08x stack %08x\n",
             (uint32_t) pcb->context, (uint32_t) pcb->stack );
}

/*
** _context_dump(msg,context)
**
** dump the contents of this process context to the console
*/
void _context_dump( const char *msg, context_t *context ) {

   c_printf( "%s @ %08x: ", msg, (uint32_t) context );
   if( context == NULL ) {
      c_puts( " NULL???\n" );
      return;
   }

   c_printf( "\n     ss %08x  gs %08x  fs %08x  es %08x\n",
             context->ss, context->gs, context->fs, context->es );
   c_printf( "     ds %08x edi %08x esi %08x ebp %08x\n",
             context->ds, context->edi, context->esi, context->ebp );
   c_printf( "    esp %08x ebx %08x edx %08x ecx %08x\n",
             context->esp, context->ebx, context->edx, context->ecx );
   c_printf( "    eax %08x vec %08x cod %08x eip %08x\n",
             context->eax, context->vector, context->code, context->eip );
   c_printf( "     cs %08x efl %08x\n", context->cs, context->eflags );
}

/*
** _active_dump(msg,all)
**
** dump the contents of the "active processes" table
*/
void _active_dump( const char *msg, int all ) {

   if( msg ) {
      c_printf( "%s:\n", msg );
   }

   int used = 0;
   int empty = 0;

   for( int i = 0; i < MAX_PROCS; ++i ) {
      if( _active[i] != NULL ) {

	 // if not dumping everything, add commas if needed
	 if( !all && used ) {
	    c_putchar( ',' );
	 }

	 // things that are always printed
         c_printf( " #%d: %d/%d", i, _active[i]->pid, _active[i]->ppid );
	 if( VALID_STATE(_active[i]->state) ) {
	    c_printf( " %s", _states[_active[i]->state] );
	 } else {
            c_printf( " ?(%d)", _active[i]->state );
	 }
	 c_printf( VALID_PRIO(_active[i]->prio) ? " pr %d" : " pr ?(%d)",
	           _active[i]->prio );

	 // do we want more info?
         if( all ) {
	    c_printf( " stk %08x len %d\n", (uint32_t) _active[i]->stack,
	       *(((uint32_t *)_active[i]->stack)-1) );
	 }

	 // one more non-empty slot
         ++used;

      } else {

         ++empty;

      }
   }

   // only need this if we're doing one-line output
   if( !all ) {
      c_putchar( '\n' );
   }

   // make sure we saw the correct number of slots in the table
   if( (used + empty) != MAX_PROCS ) {
      c_printf( "Table size %d, used %d + empty %d = %d???\n",
         MAX_PROCS, used, empty, used + empty );
   }
}
