/*
** SCCS ID:	@(#)stacks.c	1.1	3/29/18
**
** File:	stacks.c
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	Stack module implementation
*/

#define	__SP_KERNEL__

#include "common.h"

#include "stacks.h"

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

stack_t _system_stack;		// the OS stack
uint32_t *_system_esp;		// the OS %ESP value

/*
** PRIVATE FUNCTIONS
*/

/*
** PUBLIC FUNCTIONS
*/

/*
** _stk_init()
**
** initializes all stack-related data structures
*/
void _stk_init( void ) {

   c_puts( " STK" );

   /*
   ** Create the initial system ESP
   **
   ** This is the stack that will be used
   ** when ISRs are invoked.  ESP will point
   ** to the next-to-last word in the stack.
   */

   _system_esp = ((uint32_t *) ( (&_system_stack) + 1)) - 2;
   
}

/*
** _stk_alloc()
**
** allocate a stack structure
**
** returns a pointer to the stack, or NULL on failure
*/
stack_t *_stk_alloc( void ) {
   stack_t *new = NULL;

   // try to allocate one

   new = _kmalloc( sizeof(stack_t) );

   // if we can't, just return a NULL pointer

   if( new == NULL ) {
      return( NULL );
   }

   // got one - send it back to the caller

   return( new );
}

/*
** _stk_free(stack)
**
** deallocate a stack, putting it into the set of available stacks
*/
void _stk_free( stack_t *stack ) {
   
   // sanity check:  avoid deallocating a NULL pointer
   if( stack == NULL ) {
      // should this be an error?
      return;
   }

   // release it
   _kfree( stack );
}

/*
** _stk_dump(msg,stk,limit)
**
** dump the contents of the provided stack, eliding duplicate lines
**
** assumes the stack size is a multiple of four words
*/

/*
** output lines begin with the 8-digit address, followed by a hex
** interpretation of the four words and a character interpretation
**
** hex needs 41 bytes:
** col pos  1         2         3         4
** 1        0         0         0         0
**   xxxxxxxx  xxxxxxxx  xxxxxxxx  xxxxxxxx
**
** character needs 22 bytes:
**             1    1  2
** 1 3    8    3    8  1
**   cccc cccc cccc cccc
*/

// round up a bit
#define	HBUFSZ		48
#define	CBUFSZ		24

void _stk_dump( const char *msg, stack_t *stack, uint32_t limit ) {
   int words = STACK_SIZE_U32;
   int eliding = 0;
   char oldbuf[HBUFSZ], buf[HBUFSZ], cbuf[CBUFSZ];
   uint32_t addr = (uint32_t ) stack;
   uint32_t *sp = (uint32_t *) stack;
   char hexdigits[] = "0123456789ABCDEF";

   // if a limit was specified, dump only that many words

   if( limit > 0 ) {
      words = limit;
      if( (words & 0x3) != 0 ) {
         // round up to a multiple of four
	 words = (words & 0xfffffffc) + 4;
      }
      // skip to the new starting point
      sp += (STACK_SIZE_U32 - words);
      addr = (uint32_t) sp;
   }

   c_puts( "*** stack" );
   if( msg != NULL ) {
      c_printf( " (%s):\n", msg );
   } else {
      c_puts( ":\n" );
   }

   oldbuf[0] = '\0';

   while( words > 0 ) {

      register char *bp = buf;   // start of hex field
      register char *cp = cbuf;  // start of character field
      uint32_t start_addr = addr;

      // iterate through the words for this line

      for( int i = 0; i < 4; ++i ) {
         register uint32_t curr = *sp++;
	 register uint32_t data = curr;

	 // convert the hex representation

	 // two spaces before each entry
         *bp++ = ' ';
         *bp++ = ' ';

         for( int j = 0; j < 8; ++j ) {
            uint32_t value = (data >> 28) & 0xf;
            *bp++ = hexdigits[value];
            data <<= 4;
         }

	 // now, convert the character version
	 data = curr;

	 // one space before each entry
	 *cp++ = ' ';

	 for( int j = 0; j < 4; ++j ) {
	    uint32_t value = (data >> 24) & 0xff;
	    *cp++ = (value >= ' ' && value < 0x7f) ? (char) value : '.';
	    data <<= 8;
	 }
      }
      *bp = '\0';
      *cp = '\0';

      words -= 4;
      addr += 16;

      // if this line looks like the last one, skip it

      if( _kstrcmp(oldbuf,buf) == 0 ) {
         ++eliding;
         continue;
      }

      // it's different, so print it

      // start with the address
      c_printf( "%08x%c", start_addr, eliding ? '*' : ' ' );
      eliding = 0;

      // print the words
      c_printf( "%s %s\n", buf, cbuf );

      // remember this line
      _kmemcpy( (uint8_t *) oldbuf, (uint8_t *) buf, HBUFSZ );

   }
}

/*
** _stk_setup(stack,entry,argv,argc,len)
**
** set up 'stack' with a fresh user context
**
** returns a pointer to the context save area in the stack
*/
context_t *_stk_setup( stack_t *stack, uint32_t entry, char *argv[],
      int argc, int len ) {

   uint32_t *ptr;
   context_t *context;

#ifdef TRACE_STACK_SETUP
   c_printf( "== _stk_setup(%08x,%08x,%08x,%d,%d)\n", (uint32_t) stack,
      entry, (uint32_t) argv, argc, len );
   if( argv != NULL ) {
      c_printf( "== argv:  [0] %08x", (uint32_t)argv[0] );
      if( argv[0] != NULL ) c_printf( " '%s'", argv[0] );
      c_printf( ", [1] %08x", (uint32_t)argv[1] );
      if( argv[1] != NULL ) c_printf( " '%s'", argv[1] );
      c_putchar( '\n' );
   }
#endif

   // if there is no stack, we can't do anything

   if( stack == NULL ) {
      return( NULL );
   }

   // clear the whole user stack

   _kmemclr( (uint8_t *) stack, sizeof(stack_t) );

   /*
   ** Set up the initial stack contents for a (new) user process.
   **
   ** We reserve one longword at the bottom of the stack as scratch
   ** space.  Above that, we simulate a call from do_exit() with an
   ** argument vector by pushing the argv strings, then argv, then
   ** argc.  We follow this up by pushing the address of the entry point
   ** of do_exit() as a "return address".  Above that, we place a
   ** context_t area that is initialized with the standard initial register
   ** contents.
   **
   ** The low end of the stack will contain these values:
   **
   **      esp ->  ?        <- context save area
   **              ...      <- context save area
   **              ?        <- context save area
   **              do_exit  <- return address for faked call to main()
   **              argc     <- argument count for main()
   **              argv     <- argv pointer for main()
   **               ...     <- argv array w/trailing NULL
   **               ...     <- argv character strings
   **              0        <- last word in stack
   **
   ** When this process is dispatched, the context restore code
   ** will pop all the saved context information off the stack,
   ** leaving the "return address" on the stack as if the main()
   ** for the process had been "called" from the do_exit() function.
   ** When main() returns, it will "return" to the entry point of
   ** do_exit(), which will clean it up.
   */

   // find the address following the stack

   ptr = (uint32_t *) (stack + 1);

   // assign the dummy "last word"

   *--ptr = 0;

#ifdef TRACE_STACK_SETUP
   if( argv != NULL ) {
      c_printf( "== post-dummy argv %08x  [0] %08x [1] %08x\n",
         (uint32_t)argv, (uint32_t)argv[0], (uint32_t)argv[1] );
   }
#endif

   /*
   ** determine the argument count and the total string length
   **
   ** NOTE:  we do argv counting and duplicating A LOT.  We have to do
   ** this at the user level in spawn() to ensure that the argv entries
   ** aren't overwritten by the parent process if it's going to spawn()
   ** another child, and we do this in _sys_exec() because the first thing
   ** we do here is clear the stack, which also wipes out the argv strings.
   **
   ** _stk_setup() was modified to take the argument count and the total
   ** string space needed as parameters, which saves us recalculating
   ** those values here.
   */

   // copy in the argv strings and remember their addresses

   char *args[MAX_ARGUMENTS+1];

   // pointer to where the first arg string goes

   char *cptr = ((char *) ptr) - len;

   // remember that location

   char *save = cptr;

#ifdef TRACE_STACK_SETUP
   c_printf( "== args @ %08x: ", (uint32_t) cptr );
#endif
   for( int i = 0; i < argc; ++i ) {
      _kstrcpy( cptr, argv[i] );  // copy the string
      args[i] = cptr;             // remember where it went
#ifdef TRACE_STACK_SETUP
      c_printf( " '%s' -> %08x", cptr, (uint32_t) cptr );
#endif
      cptr += _kstrlen( argv[i] ) + 1;  // move past it
   }

   // remember the NULL!

   args[argc] = NULL;

   /*
   ** start placing the argv entries into the stack.
   **
   ** first, round the address down to ensure that it's aligned properly.
   */

#ifdef TRACE_STACK_SETUP
   c_printf( "\n== orig cptr %08x", (uint32_t) save );
#endif
   while( ((uint32_t) save) & 3 ) {
      --save;
   }
#ifdef TRACE_STACK_SETUP
   c_printf( " rounded %08x\n", (uint32_t) save );
#endif

   // now, copy in the argv entries in reverse order, starting with the NULL

   ptr = (uint32_t *) save;

   for( int i = argc; i >= 0; --i ) {
      *--ptr = (uint32_t) args[i];
#ifdef TRACE_STACK_SETUP
      c_printf( ", av[%d] %08x to %08x", i, (uint32_t) args[i],
         (uint32_t) ptr );
#endif
   }

   // remember the argv[0] address

   uint32_t *tmp = ptr;

   // now push argv and then argc

   *--ptr = (uint32_t) tmp;
#ifdef TRACE_STACK_SETUP
   c_printf( "\n== argv %08x to %08x, ", (uint32_t) tmp, (uint32_t) ptr );
#endif
   *--ptr = argc;
#ifdef TRACE_STACK_SETUP
   c_printf( "argc %d to %08x\n", argc, (uint32_t) ptr );
#endif

   // push the "return address"
   *--ptr = (uint32_t) do_exit;

   // now, set up the process context
   context = ((context_t *) ptr) - 1;

   // initialize all the registers that should be non-zero
   context->eflags = DEFAULT_EFLAGS;
   context->eip = entry;
   context->ebp = 0;  // end of EBP stack frame chain
   context->cs = GDT_CODE;
   context->ss = GDT_STACK;
   context->ds = GDT_DATA;
   context->es = GDT_DATA;
   context->fs = GDT_DATA;
   context->gs = GDT_DATA;

   // all done!
   return( context );
}
