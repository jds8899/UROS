/*
** SCCS ID:   @(#)clock.c	1.1   3/29/18
**
** File:   clock.c
**
** Author:   Warren R. Carithers and various CSCI-452 sections
**
** Contributor:
**
** Description:   Clock module implementation
*/

#define   __SP_KERNEL__

#include <x86arch.h>
#include "startup.h"

#include "common.h"

#include "clock.h"
#include "pcbs.h"
#include "queues.h"
#include "scheduler.h"
#include "sio.h"
#include "syscalls.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

// pinwheel control variables

static int32_t _pinwheel;   // pinwheel counter
static uint32_t _pindex;   // index into pinwheel string

/*
** PUBLIC GLOBAL VARIABLES
*/

uint32_t _system_time;      // the current system time

/*
** PRIVATE FUNCTIONS
*/


/*
** _clk_isr(vector,ecode)
**
** Interrupt handler for the clock module.  Spins the pinwheel,
** wakes up sleeping processes, and handles quantum expiration
** for the current process.
*/
static void _clk_isr( int vector, int ecode ) {
   pcb_t *pcb;

   // spin the pinwheel

   ++_pinwheel;
   if( _pinwheel == (CLOCK_FREQUENCY / 10) ) {
      _pinwheel = 0;
      ++_pindex;
      c_putchar_at( 79, 0, "|/-\\"[ _pindex & 3 ] );
   }

#ifdef DUMP_QUEUES
   // Approximately every 10 seconds, dump the queues, and
   // print the contents of the SIO buffers and the active table

   if( (_system_time % SECONDS_TO_TICKS(10)) == 0 ) {
#ifdef DUMP_QUEUE_CONTENTS
      // dump the full contents
      _q_dump_all();
#else
      // just dump the sizes
      c_printf( "Queues @ %08x:", _system_time );
      c_printf( "  sleep %d wait %d read %d zombie %d",
         _q_size(_sleeping), _q_size(_waiting),
         _q_size(_reading), _q_size(_zombie) );
      c_puts( " ready [" );
      for( int i = 0; i < N_PRIOS; ++i ) {
         c_printf( "%d", _q_size(_ready[i]) );
         if( i != (N_PRIOS-1) ) {
            c_putchar( ',' );
         }
      }
      c_puts( "]\n" );
#endif
      _sio_dump();
      _active_dump( "Process table", FALSE );
   }
#endif

   // increment the system time

   ++_system_time;

   /*
   ** wake up any sleeper whose time has come
   **
   ** we give awakened processes preference over the
   ** current process (when it is scheduled again)
   */

   pcb = (pcb_t *) _q_peek( _sleeping );
   while( pcb != NULL && pcb->event <= _system_time ) {

      // time to wake up!  remove it from the queue
      pcb = (pcb_t *) _q_remove( _sleeping );
      if( pcb == NULL ) {
         // it's there, but we can't remove it???
         _kpanic( "_clk_isr", "sleep queue remove failure" );
      }

      // and schedule it for dispatch
      _schedule( pcb );

      // see if there's another one to wake up
      pcb = (pcb_t *) _q_peek( _sleeping );
   }
   
   // check the current process to see if it needs to be scheduled

   _current->ticks -= 1;
   if( _current->ticks < 1 ) {
      _schedule( _current );
      _dispatch();
   }

   // tell the PIC we're done

   __outb( PIC_MASTER_CMD_PORT, PIC_EOI );
}

/*
** PUBLIC FUNCTIONS
*/

/*
** _clk_init()
**
** initialize the clock module
*/

void _clk_init( void ) {
   uint32_t divisor;

        // announce that we got this far

        c_puts( " CLOCK" );

   // start the pinwheel

   _pinwheel = (CLOCK_FREQUENCY / 10) - 1;
   _pindex = 0;

   // return to the epoch

   _system_time = 0;

   // set the clock to tick at CLOCK_FREQUENCY Hz.

   divisor = TIMER_FREQUENCY / CLOCK_FREQUENCY;
   __outb( TIMER_CONTROL_PORT, TIMER_0_LOAD | TIMER_0_SQUARE );
        __outb( TIMER_0_PORT, divisor & 0xff );        // LSB of divisor
        __outb( TIMER_0_PORT, (divisor >> 8) & 0xff ); // MSB of divisor

   // register the ISR

   __install_isr( INT_VEC_TIMER, _clk_isr );
}
