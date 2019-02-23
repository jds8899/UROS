/*
** SCCS ID:	@(#)syscalls.c	1.1	3/29/18
**
** File:	syscalls.c
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	System call module implementations
*/

#define	__SP_KERNEL__

#include "common.h"

#include <x86arch.h>
#include <uart.h>

#include "support.h"
#include "startup.h"

#include "syscalls.h"
#include "scheduler.h"
#include "pcbs.h"
#include "stacks.h"
#include "clock.h"
#include "sio.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

// system call jump table
//
// initialized by _sys_init() to ensure that the
// code::function mappings are correct

static void (*_syscalls[N_SYSCALLS])( void );

// pointers to all process queues
//
// must have one slot for each state, plus extras
// for the additional ready queues

static queue_t _all_queues[ N_STATES + N_PRIOS - 1 ];

/*
** PUBLIC GLOBAL VARIABLES
*/

/*
** PRIVATE FUNCTIONS
*/

/*
** _sys_isr(vector,ecode)
**
** Common handler for the system call module.  Selects
** the correct second-level routine to invoke based on
** the contents of EAX.
**
** The second-level routine is invoked with a pointer to
** the PCB for the process.  It is the responsibility of
** that routine to assign all return values for the call.
*/
static void _sys_isr( int vector, int ecode ) {
   uint32_t code;
   
   // which one does the user want?

   code = REG(_current,eax);
   
   // verify that it's legal - if not, it's adios, muchacho!

   if( code >= N_SYSCALLS ) {
      code = SYS_exit;
      ARG(_current,1) = EXIT_BAD_CODE;
   }
   
   // invoke it

   _syscalls[code]();
   
   // notify the PIC

   __outb( PIC_MASTER_CMD_PORT, PIC_EOI );
}

/*
** Second-level syscall handlers
**
** All have this prototype:
**
**      static void _sys_NAME( void );
**
** Values being returned to the user are placed into the EAX
** field in the context save area of the provided PCB.
*/

/*
** _sys_time - retrieve the current system time
**
** implements:  uint32_t time( void );
**
** returns:
**      the current system time in milliseconds
*/
static void _sys_time( void ) {
   RET(_current) = _system_time / TICKS_PER_MS;
}

/*
** _sys_pid - retrieve the PID of the current process
**
** implements:  uint16_t pid( void );
**
** returns:
**      the PID of this process
*/
static void _sys_pid( void ) {
   RET(_current) = _current->pid;
}

/*
** _sys_ppid - retrieve the parent PID of the current process
**
** implements:  uint16_t ppid( void );
**
** returns:
**      the PID of the parent of this process
*/
static void _sys_ppid( void ) {
   RET(_current) = _current->ppid;
}

/*
** _sys_get_prio - retrieve the priority of the specified process
**
** implements:  int get_prio( uint16_t whom );
**
** returns:
**      the priority of the process as an integer, or an error code
*/
static void _sys_get_prio( void ) {
   uint16_t whom = ARG(_current,1);
   
   // alias for the current process

   if( whom == 0 ) {
      whom = _current->pid;
   }
   
   // locate the desired PCB

   pcb_t *pcb = _pcb_find( whom );
   
   // if we found it, return its priority; otherwise, an error code

   RET(_current) = pcb ? pcb->prio : E_BAD_PID;
}

/*
** _sys_set_prio - set the priority of the specified process
**
** implements:  int set_prio( uint16_t whom, uint8_t prio );
**
** returns:
**      the old priority of the process as an integer, or an error code
*/
static void _sys_set_prio( void ) {
   uint16_t whom = ARG(_current,1);
   uint8_t prio = ARG(_current,2);
   
   // interpret 0 as a reference to the current process

   if( whom == 0 ) {
      whom = _current->pid;
   }
   
   // validate the supplied priority

   if( prio < PRIO_MIN || prio > PRIO_MAX ) {
      RET(_current) = E_BAD_PRIO;
      return;
   }
   
   // locate the lucky winner

   pcb_t *pcb = _pcb_find( whom );
   
   // did we find it?

   if( pcb == NULL ) {
      // nope!
      RET(_current) = E_BAD_PID;
      return;
   }
   
   // found it - remember its old priority, and assign the new one

   uint8_t oldprio = pcb->prio;
   pcb->prio = prio;
   
   // return the old priority value
   RET(_current) = oldprio;
}

/*
** _sys_sleep - put the current process to sleep
**
** implements:  void sleep( uint32_t msec );
**
** notes:
**      interprets a time of 0 as a request to yield the CPU
*/
static void _sys_sleep( void ) {
   uint32_t ms = ARG(_current,1);
   
   // determine the wakeup time

   if( ms == 0 ) {

      // equivalent to yield()

      _schedule( _current );

   } else {

      // calculate the wakeup time

      uint32_t wakeup = (ms * TICKS_PER_MS) + _system_time;
      
      // change the state and remember the wakeup time

      _current->state = ST_SLEEPING;
      _current->event = wakeup;
      
      // nitey-nite!

      _q_insert( _sleeping, (void *) _current );
   }
   
   // either way, no current process, so we need to pick one

   _dispatch();
}

/*
** _sys_fork - create a new process
**
** implements:  int fork( void );
**
** returns:
**   parent:  PID of the new child, or an error code
**   child:   0
*/
static void _sys_fork( void ) {
   pcb_t *new;
   stack_t *stack;
   
   // make sure we have room for the process

   if( _num_active >= MAX_PROCS ) {
      RET(_current) = E_TOO_MANY_PROCS;
      return;
   }

   /*
   ** have we run out of PIDs?
   **
   ** we should "wrap" our PIDs once we hit 30K to 60K,
   ** but that would require writing some code to figure
   ** out which PIDs were available (to avoid having 
   ** two or more processes with the same PID) and would
   ** complicate the "assign the next available PID" code
   */

   if( _next_pid >= 65000U ) {
      RET(_current) = E_NO_MORE_PROCS;
      c_printf( "*** next pid is %d\n", _next_pid );
      c_puts( "*** no more processes can be created!\n" );
      return;
   }
   
   // allocate a PCB for this process

   new = _pcb_alloc();
   if( new == NULL ) {
      RET(_current) = E_NO_PCBS;
      return;
   }
   
   // give it a stack

   stack = _stk_alloc();
   if( stack == NULL ) {
      _pcb_free( new );
      RET(_current) = E_NO_STACKS;
      return;
   }
#if TRACE_SPAWN > 1
   c_printf( ">> fork(), current PCB %08x stack %08x",
      (uint32_t) _current, (uint32_t) _current->stack );
   c_printf( ", new PCB %08x stack %08x\n", (uint32_t) new, (uint32_t) stack );
#endif
   // duplicate the parent's PCB

   _kmemcpy( (uint8_t *) new, (uint8_t *) _current, sizeof(pcb_t) );

   // fix the fields that need to be changed

   new->pid      = _next_pid++;
   new->ppid     = _current->pid;
   new->stack    = stack;
   new->children = 0;
   
   // duplicate the parent's stack

   _kmemcpy( (uint8_t *)stack, (uint8_t *)(_current->stack), sizeof(stack_t) );
   
   /*
   ** now, we need to update the ESP and EBP values in the
   ** child so that they don't point into the parent's stack.
   */

   // figure out the byte offset from one stack to the other

   int32_t offset = (int)
      ( ((void *)new->stack) - ((void *)_current->stack) );
      
   // add this to the child's context pointer

   new->context = (context_t *)
      ( ((void *)_current->context) + offset );
      
   // fix the child's EBP and ESP values

   REG(new,ebp) += offset;
   REG(new,esp)  = (uint32_t) new->context;
   
   // follow the EBP chain through the child's stack

   uint32_t *bp = (uint32_t *) REG(new,ebp);
   while( bp && *bp ) {
      *bp += offset;
      bp = (uint32_t *) *bp;
   }
   
   // add the new process to the active table

   int i;
   for( i = 0; i < MAX_PROCS; ++i ) {
      if( _active[i] == NULL ) {
         _active[i] = new;
         break;
      }
   }
   
   // should not run out of room, as we checked for this earlier

   if( i >= MAX_PROCS ) {
      _kpanic( "_sys_fork", "no slot in non-full _active" );
   }
   
   _num_active += 1;
   
   // assign the various return values

   RET(new) = 0;
   RET(_current) = new->pid;

   // increment the child count for the parent

   _current->children += 1;
   
   // schedule the child, and let the parent continue executing

#if CHILD_FIRST
   // switch to the new process
   _schedule( _current );
   _current = new;
   // give it just a little time
   new->ticks = QUANTUM_SHORT;
#else
   _schedule( new );
#endif
}

/*
** _sys_exec - replace this program with a different one
**
** implements:  int exec( int (*entry)(int, char **), char *argv[] );
**
** returns:
**      on success, doesn't return
**   else, returns an error code
*/
static void _sys_exec( void ) {
   uint32_t entry = ARG(_current,1);
   char **argv = (char **) ARG(_current,2);
   
   // clean up the stack and set it up for the process

#ifdef TRACE_EXEC
   c_printf( "++ _sys_exec: curr %08x pid %d entry %08x stk %08x\n",
      (uint32_t) _current, _current->pid, entry,
      (uint32_t) _current->stack );
   c_printf( "++ argv %08x", (uint32_t) argv );
   if( argv != NULL ) {
      c_printf( ", [0] %08x", (uint32_t) (argv[0]) );
      if( argv[0] ) c_printf( " '%s'", argv[0] );
      c_printf( ", [1] %08x", (uint32_t) (argv[1]) );
      if( argv[1] ) c_printf( " '%s'", argv[1] );
      c_putchar( '\n' );
   } else {
      c_puts( " (NULL!)\n" );
   }
#endif

   /*
   ** We must duplicate the argv array here, because the first
   ** thing _stk_setup() does is clear the stack, which wipes
   ** out the argument vector.  This was never a problem in the
   ** past, because the baseline OS never supported passing
   ** argument vectors to user processes.  D'oh!
   **
   ** Duplicate the argv strings and argv points.  Unlike the
   ** user-level spawn() function, here we can use dynamic arrays
   ** because this storage is all on the OS stack, not the user
   ** stack.
   */

   // figure out how many arguments & argument chars there are

   int argc = 0;
   int len = 0;

   while( argv[argc] != NULL ) {
      len += _kstrlen( argv[argc] ) + 1;
      ++argc;
   }

   // duplicate everything

   char strtbl[len];
   char *args[argc+1];

   char *ptr = strtbl;

   for( int i = 0; i < argc; ++i ) {
      _kstrcpy( ptr, argv[i] );
      args[i] = ptr;
      ptr += _kstrlen( argv[i] ) + 1;  // don't forget the NUL!
   }

   // add the NULL

   args[argc] = NULL;

#ifdef TRACE_EXEC
   c_printf( "++ pre-setup len %d argc %d args %08x", len, argc,
      (uint32_t) args );
   c_printf( ", [0] %08x", (uint32_t) (args[0]) );
   if( args[0] ) c_printf( " '%s'", args[0] );
   c_printf( ", [1] %08x", (uint32_t) (args[1]) );
   if( args[1] ) c_printf( " '%s'", args[1] );
   c_putchar( '\n' );
#endif

   // off we go!

   _current->context = _stk_setup( _current->stack, entry, args, argc, len );

#ifdef TRACE_EXEC
   c_printf( "++ _sys_exec: curr %08x pid %d entry %08x stk %08x ctxt %08x\n",
      (uint32_t) _current, _current->pid, entry,
      (uint32_t) _current->stack, (uint32_t) _current->context );
   _stk_dump( NULL, _current->stack, 12 );
   _shell(); // invoke the interactive shell to pause things
#endif

   if( _current->context == NULL ) {
      c_printf( "*** exec: pid %d stk %08x got NULL context\n",
         _current->pid, (uint32_t) _current->stack );
      _kpanic( "_sys_exec", "NULL context from _stk_setup" );
   }

   /*
   ** question:  should we schedule and dispatch here?
   ** we choose not to, so the "new" program will
   ** begin executing immediately, with whatever's left
   ** of the "old" quantum for the process.
   */
}

/*
** _sys_kill - terminate a process with extreme prejudice
**
** implements:  int32_t kill( uint16_t pid );
**
** returns:
**      E_NO_ERROR if found
**      E_BAD_PID if not found
*/
static void _sys_kill( void ) {
   uint16_t pid = ARG(_current,1);
   pcb_t *pcb = NULL;
   
   // interpret 0 as "ourselves"

   if( pid == 0 ) {
      pid = _current->pid;
   }
   
   // find the PCB for the victim

   if( pid == _current->pid ) {
      pcb = _current;
   } else {
      pcb = _pcb_find( pid );
      if( pcb == NULL ) {
         RET(_current) = E_BAD_PID;
         return;
      }
   }
   
   // if there isn't one, something seriously bad has happened

   if( pcb == NULL ) {
      _kpanic( "_sys_kill", "no victim PCB (can't happen?)" );
   }
   
   // verify that the victim's state is legal

   if( pcb->state < FIRST_STATE || pcb->state > LAST_STATE ) {
      c_printf( "ERR: kill(%d), victim state %d\n", pcb->pid, pcb->state );
      _kpanic( "_sys_kill", "bad state on victim" );
   }
   
   // if the victim is already dead, can't kill it again

   if( pcb->state == ST_KILLED || pcb->state == ST_ZOMBIE ) {
      RET(_current) = E_UNDEAD;
      return;
   }
   
   /*
   ** this code is somewhat grotesque.
   **
   ** the '_all_queues' array contains pointers to the queue_t 
   ** objects for each of the various queues.  in the case of 
   ** the ready queue, there are actually N_PRIOS queues to deal
   ** with, selected by process priority.  we deal with this by
   ** putting the ready queues at the end of _all_queues, and
   ** using the priority value as an index into the subarray
   ** containing those queue_t pointers.
   */

   queue_t which;

   // find the queue containing the victim

   if( pcb->state == ST_READY ) {
      which = _all_queues[ ST_READY + pcb->prio ];
   } else {
      which = _all_queues[ pcb->state ];
   }
   
   // assuming there is one, remove the victim from the queue

   if( which != NULL ) {
      pcb_t *victim = _q_remove_by( which, (void *) (uint32_t) pcb->pid,
                          _q_find_pid );

      // verify that it's the correct victim
      if( victim != pcb ) {
         c_printf( "ERR: kill(%d), orig PCB %x victim PCB %x???\n",
       pcb->pid, (uint32_t) pcb, (uint32_t) victim );
         _kpanic( "_sys_kill", "wrong victim found???" );
      }
   }
   
   // mark the victim

   pcb->state = ST_KILLED;
   pcb->exitstatus = EXIT_KILLED;
   RET(_current) = 0;
   
   // and make it undead

   _bite( pcb );
   
   // if this process killed itself, need a new one

   if( pcb == _current ) {
      _dispatch();
   }
}

/*
** _sys_exit - terminate the calling process
**
** implements:  void exit( int32_t status );
**
** does not return
*/
static void _sys_exit( void ) {
   int32_t status = ARG(_current,1);
   pcb_t *pcb = _current;
   
   // remember the exit status

   pcb->exitstatus = status;
   
   // make it undead

   _bite( pcb );
   
   // pick a new winner

   _dispatch();
}

/*
** _sys_read - read from an input channel into a fixed-sized buffer
**
** implements:   int read( int channel, char *buffer, uint32_t length );
**
** blocks the calling routine if there is no character to return
**
** returns:
**   the count of characters read, or an error code
*/
static void _sys_read( void ) {
   int n = 0;
   int chan = ARG(_current,1);
   char *buf = (char *) ARG(_current,2);
   int length = ARG(_current,3);

   // try to get the next character

   switch( chan ) {

      case CHAN_CONSOLE:
         // console input doesn't block
         if( c_input_queue() < 1 ) {
            RET(_current) = E_NO_DATA;
            return;
         }
         n = c_gets( buf, length );
         break;

      case CHAN_SIO:
         // this may block
         n = _sio_reads( buf, length );
         break;

      default:
         // bad channel code!
         RET(_current) = E_BAD_CHAN;
         return;
   }

   // if there was a character, return the count to the process;
   // otherwise, block the process until one comes in

   if( n > 0 ) {

      RET(_current) = n;

   } else {

      // no character; put this process on the
      // serial i/o input queue

      _q_insert( _reading, _current );

      // mark it as blocked

      _current->state = ST_BLOCKED_IO;

      // select a new current process

      _dispatch();

   }

}

/*
** _sys_write - write a sized buffer to a channel
**
** implements:   int write( int channel, const char *buf, uint32_t length );
**
** returns:
**   the count of characters transferred, or an error code
*/
static void _sys_write( void ) {
   int chan = ARG(_current,1);
   const char *buf = (const char *) ARG(_current,2);
   int length = ARG(_current,3);

   // this is almost insanely simple, but it does separate
   // the low-level device access fromm the higher-level
   // syscall implementation

   switch( chan ) {

      case CHAN_CONSOLE:
         // the c_io module doesn't have a "write N characters" function
         for( int i = 0; i < length; ++i ) {
            c_putchar( buf[i] );
         }
         RET(_current) = length;
         break;

      case CHAN_SIO:
         // much simpler!
         _sio_writes( buf, length );
         RET(_current) = length;
         break;

      default:
         RET(_current) = E_BAD_CHAN;
         break;

   }

}

/*
** _sys_wait - wait for one of the children of a process
**
** implements:  int wait( int32_t *status );
**
** returns:
**      on error, E_NO_KIDS
**      on success, the pid and status of the child
*/
static void _sys_wait( void ) {

   // verify that there are children to wait for

   if( _current->children < 1 ) {
      RET(_current) = E_NO_KIDS;
      return;
   }

   // find the first zombie child

   pcb_t *child = _q_remove_by( _zombie, (void *) (uint32_t) _current->pid,
               _q_find_ppid );

   // if there wasn't one, block the process until one appears;
   // otherwise, collect it's status and clean it up

   if( child == NULL ) {

      // no undead children, so we must wait
      _current->state = ST_WAITING;
      _q_insert( _waiting, _current );

      // let someone else run
      _dispatch();

   } else {

      // found one - collect and return its exit status

      RET(_current) = child->pid;
      int32_t *status = (int32_t *) ARG(_current,1);
      *status = child->exitstatus;

      // one fewer child

      _current->children -= 1;

      // clean up the child

      _pcb_cleanup( child );
   }
}

/*
** _sys_dumpme - dump all information about a user process
**
** implements:  void dumpme( int fatal );
*/
static void _sys_dumpme( void ) {
   int fatal = ARG(_current,1);

   _pcb_dump( "*** PROCESS DUMP REQUESTED", _current );
   _context_dump( "*** Context", _current->context );
   _stk_dump( "*** Stack", _current->stack, 0 );

   if( fatal ) {
      c_puts( "*** LOCKUP REQUESTED ***\n" );
      for(;;);
   } else {
      // just delay
      __delay( 200 );
   }
}

/*
** PUBLIC FUNCTIONS
*/

/*
** _sys_init()
**
** initialize the syscall module
**
** MUST BE CALLED AFTER THE _sio_init AND _sched_init FUNCTIONS
** HAVE BEEN CALLED, SO THAT THE _reading QUEUE HAS BEEN CREATED,
** AND _all_queues CAN BE INITIALIZED
*/
void _sys_init( void ) {

   // report that we made it this far
   c_puts( " SYSCALL" );

   /*
   ** Set up the syscall jump table.  We do this here
   ** to ensure that the association between syscall
   ** code and function address is correct even if the
   ** codes change.
   */

   _syscalls[ SYS_exit ]     = _sys_exit;
   _syscalls[ SYS_read ]     = _sys_read;
   _syscalls[ SYS_write ]    = _sys_write;
   _syscalls[ SYS_fork ]     = _sys_fork;
   _syscalls[ SYS_exec ]     = _sys_exec;
   _syscalls[ SYS_kill ]     = _sys_kill;
   _syscalls[ SYS_wait ]     = _sys_wait;
   _syscalls[ SYS_sleep ]    = _sys_sleep;
   _syscalls[ SYS_time ]     = _sys_time;
   _syscalls[ SYS_pid ]      = _sys_pid;
   _syscalls[ SYS_ppid ]     = _sys_ppid;
   _syscalls[ SYS_get_prio ] = _sys_get_prio;
   _syscalls[ SYS_set_prio ] = _sys_set_prio;
   _syscalls[ SYS_dumpme ]   = _sys_dumpme;

   /*
   ** initialize the list of all process queues
   **
   ** NULL indicates that there is no queue associated with this state
   **
   ** the ready queue is a MLQ structure, so we need multiple entries
   ** for it
   */

   _all_queues[ ST_UNUSED ]     = NULL;
   _all_queues[ ST_NEW ]        = NULL;
   _all_queues[ ST_RUNNING ]    = NULL;
   _all_queues[ ST_SLEEPING ]   = _sleeping;
   _all_queues[ ST_WAITING ]    = _waiting;
   _all_queues[ ST_BLOCKED_IO ] = _reading;
   _all_queues[ ST_KILLED ]     = _zombie;
   _all_queues[ ST_ZOMBIE ]     = _zombie;

      // these must be done in "increasing value" order
   _all_queues[ ST_READY + PRIO_SYS_HIGH  ]  = _ready[PRIO_SYS_HIGH];
   _all_queues[ ST_READY + PRIO_SYS_STD  ]   = _ready[PRIO_SYS_STD];
   _all_queues[ ST_READY + PRIO_USER_HIGH  ] = _ready[PRIO_USER_HIGH];
   _all_queues[ ST_READY + PRIO_USER_STD  ]  = _ready[PRIO_USER_STD];
   _all_queues[ ST_READY + PRIO_LOWEST  ]    = _ready[PRIO_LOWEST];

   // install the second-stage ISR

   __install_isr( INT_VEC_SYSCALL, _sys_isr );
}
