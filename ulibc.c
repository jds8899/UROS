/*
** SCCS ID:	@(#)ulibc.c	1.1	3/29/18
**
** File:	ulibc.c
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	C implementations of user-level library functions
*/

#include "common.h"

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
** First, some general utility functions
*/

/*
** cvt_dec0
**
** local support routine for cvt_dec()
**
** copied from c_io.c cvtdec0() routine
*/
char *cvt_dec0( char *buf, int value ) {
   int quotient;

   quotient = value / 10;
   if( quotient < 0 ) {
      quotient = 214748364;
      value = 8;
   }
   if( quotient != 0 ) {
      buf = cvt_dec0( buf, quotient );
   }
   *buf++ = value % 10 + '0';
   return buf;
}

/*
** cvt_dec
**
** convert a 32-bit signed value into a NUL-terminated character string
**
** returns the number of characters placed into the buffer
** (not including the NUL)
**
** copied from c_io.c cvtdec() routine
*/
int cvt_dec( char *buf, int32_t value ) {
   char *bp = buf;

   if( value < 0 ) {
      *bp++ = '-';
      value = -value;
   }

   bp = cvt_dec0( bp, value );
   *bp  = '\0';

   return( bp - buf );
}

/*
** cvt_hex
**
** convert a 32-bit value into an (up to) 8-character
** NUL-terminated character string
**
** returns the number of characters placed into the buffer
** (not including the NUL)
**
** copied from c_io.c cvthex() routine
*/
int cvt_hex( char *buf, uint32_t value ) {
   char hexdigits[] = "0123456789ABCDEF";
   int chars_stored = 0;

   for( int i = 0; i < 8; i += 1 ) {
      int val = value & 0xf0000000;
      if( chars_stored || val != 0 || i == 7 ) {
         ++chars_stored;
         val = (val >> 28) & 0xf;
         *buf++ = hexdigits[val];
      }
      value <<= 4;
   }

   *buf = '\0';

   return( chars_stored );
}

/*
** report(ch,pid)
**
** report to the console that user 'ch' is running as 'pid'
*/
void report( char ch, uint16_t whom ) {
   int i;

   char buf[20];  // up to " x(nnnnnnnnnnnnnnn)"

   buf[0] = ' ';
   buf[1] = ch;
   buf[2] = '(';
   i = cvt_dec( buf+3, whom );
   buf[3+i] = ')';
   buf[3+i+1] = '\0';
   cwrites( buf );  // one syscall vs. five
}

/*
** argdump(prefix,argv,mode)
**
** dump out a report of the user character and argv contents
*/
void argdump( const char *prefix, const char *argv[], int mode ) {
   char buf[256];
   char b2[32];
   int argc;

   if( prefix ) {
      strcpy( buf, prefix );
   } else {
      buf[0] = '\0';
   }

   for( argc = 0; argv[argc]; ++argc ) {
      ;
   }

   cvt_dec( b2, argc );
   strcat( buf, b2 );

   strcat( buf, " argv @ " );
   cvt_hex( b2, (uint32_t) argv );
   strcat( buf, b2 );

   if( argc > 0 ) {

      if( mode ) {  // dump arg strings, too
         
	 strcat( buf, "\n" );
	 cwrites( buf );

	 for( int i = 0; i < argc; ++i ) {
	    strcpy( buf, "  " );
	    int n = cvt_hex( b2, (uint32_t) argv[i] );
	    while( n < 8 ) {  // zero-fill the prefix
	       strcat( buf, "0" );
	       ++n;
	    }
	    strcat( buf, b2 );
	    strcat( buf, " \"" );
	    strcat( buf, argv[i] );
	    strcat( buf, "\"" );
	    cwrites( buf );
	 }

	 cwritech( '\n' );

      } else {      // just dump addresses of 1st two args

         strcat( buf, ": " );
         cvt_hex( b2, (uint32_t) argv[0] );
         strcat( buf, b2 );
         if( argc > 1 ) {
            strcat( buf, ", " );
	    cvt_hex( b2, (uint32_t) argv[1] );
	    strcat( buf, b2 );
            if( argc > 2 ) {
	       strcat( buf, ", ..." );
	    }
         }
         strcat( buf, "\n" );
         cwrites( buf );

      }

   } else {
      // just print what we have
      strcat( buf, " (no arg strings)\n" );
      cwrites( buf );
   }
}

/*
** Some useful C library string routines
*/

/*
** strlen()
**
** return length of a NUL-terminated string
*/
uint32_t strlen( register const char *str ) {
   register uint32_t len = 0;

   while( *str++ ) {
      ++len;
   }

   return( len );
}

/*
** strcpy - copy a NUL-terminated string
*/
char *strcpy( register char *dst, register const char *src ) {
   register char *tmp = dst;

   while( (*dst++ = *src++) )
      ;

   return( tmp );
}

/*
** strcat - append one string to another
*/
char *strcat( register char *dst, register const char *src ) {
   register char *tmp = dst;

   while( *dst )  // find the NUL
      ++dst;

   while( (*dst++ = *src++) )  // append the src string
      ;

   return( tmp );
}

/*
** strcmp - compare two NUL-terminated strings
*/
int strcmp( register const char *s1, register const char *s2 ) {

   while( *s1 != 0 && (*s1 == *s2) )
      ++s1, ++s2;

   return( *(const unsigned char *)s1 - *(const unsigned char *)s2 );
}

/*
** Next, some simpler character- and buffer-oriented i/o
** functions for use by user code.
*/

/*
** creadch()
**
** read a single character from the console
*/
int creadch( void ) {
   char buf;
   return( read(CHAN_CONSOLE,&buf,1) );
}

/*
** cread(buf,size)
**
** read into a buffer from the console
*/
int cread( char *buf, uint32_t size ) {
   return( read(CHAN_CONSOLE,buf,size) );
}

/*
** sreadch()
**
** read a single character from the SIO
*/
int sreadch( void ) {
   char buf;
   return( read(CHAN_SIO,&buf,1) );
}

/*
** sread(buf,size)
**
** read into a buffer from the SIO
*/
int sread( char *buf, uint32_t size ) {
   return( read(CHAN_SIO,buf,size) );
}

/*
** cwritech(ch)
**
** write a single character to the console
*/
int cwritech( char ch ) {
   return( write(CHAN_CONSOLE,&ch,1) );
}

/*
** cwrites(str)
**
** write a NUL-terminated string to the console
*/
int cwrites( const char *str ) {
   int len = strlen(str);
   return( write(CHAN_CONSOLE,str,len) );
}

/*
** cwrite(buf,size)
**
** write a sized buffer to the console
*/
int cwrite( const char *buf, uint32_t size ) {
   return( write(CHAN_CONSOLE,buf,size) );
}

/*
** swritech(ch)
**
** write a single character to the SIO
*/
int swritech( char ch ) {
   return( write(CHAN_SIO,&ch,1) );
}

/*
** swrites(str)
**
** write a NUL-terminated string to the SIO
*/
int swrites( const char *str ) {
   int len = strlen(str);
   return( write(CHAN_SIO,str,len) );
}

/*
** swrite(buf,size)
**
** write a sized buffer to the SIO
*/
int swrite( const char *buf, uint32_t size ) {
   return( write(CHAN_SIO,buf,size) );
}

/*
** Finally, a simpler process creation method
*/

/*
** spawn(entry,arg,prio)
**
** Creates a new process, changes its priority, and then execs 'entry'
*/
int spawn( int (*entry)(int a,char *v[]), char *argv[], uint8_t prio ) {
   int new;
   char buf[32];

   /*
   ** We need to create a string table to contain the argv[]
   ** strings, because the baseline OS doesn't use VM.
   **
   ** When a process calls spawn(), it passes in an argument
   ** vector which is (usually) one of its local variables.
   ** This means that the argv[] entries are typically pointing
   ** into the caller's stack.  After the fork(), the new child
   ** has its own stack, but the data copied into it is identical
   ** to the data in the parent's stack, which means that these
   ** pointers are all pointing back to the parent's stack.
   **
   ** If/when the parent changes those strings, they also change
   ** in the child; this lead to a problem when init() created a
   ** bunch of children in a row, because none of the children
   ** (who were all at a lower priority than init()) ran until
   ** init() finished, and thus all of their argv strings were
   ** whatever the last values assigned to that storage in init()
   ** happened to be.
   **
   ** Ideally, we would like to be able to replicate the argv
   ** strings so that the child would have them in its own stack
   ** space.  However, we also don't have a user-level memory
   ** allocator, so we need to duplicate them into the stack.
   **
   ** The obvious approach is to figure out how much space we need
   ** and then create a dynamic array (e.g., "char foo[n];").
   ** However, that won't work, because GCC "helpfully" converts "char
   ** foo[n];" into "char *foo;" and makes it point to the additional
   ** stack space it allocated.  After the fork(), this means that
   ** 'foo' in the child is STILL pointing back to the parent's stack.
   **
   ** Instead, we choose to limit the argument vector strings to a
   ** maximum of MAX_ARGV_CHARS bytes (including the trailing NUL
   ** bytes).  This is done before the fork(), but because it's just
   ** a static array of char, the child's copy after fork() is no
   ** longer linked to the parent's stack.
   **
   ** We also limit processes to no more than MAX_ARGUMENTS argv entries.
   **
   ** Grrr.
   */

   int argc;
   int len = 0;

#ifdef TRACE_SPAWN
#include "c_io.h"
   c_printf( ":: spawn(%08x,%08x,%d), called from %08x\n",
      (uint32_t) entry, (uint32_t) argv, get_ra() );
   argdump( ":: args: ", (const char **) argv, TRUE );
#endif
   // calculate the necessary length

   for( argc = 0; argv[argc]; ++argc ) {
      len = strlen( argv[argc] ) + 1;
   }
#ifdef TRACE_SPAWN
   c_printf( " -> %d args\n", argc );
#endif

   // verify that the argument vector is legal

   if( len > MAX_ARGV_CHARS ) {
      return( E_TOO_MANY_ARG_CHARS );
   }

   if( argc > MAX_ARGUMENTS ) {
      return( E_TOO_MANY_ARGS );
   }

   // copy in the args

   char strtbl[MAX_ARGV_CHARS];
   char *ptr = strtbl;
#if TRACE_SPAWN >= 2
   c_printf( ":: creating strtbl @ %08x\n", (uint32_t) ptr );
#endif

   for( int i = 0; i < argc; ++i ) {
      strcpy( ptr, argv[i] );
      ptr += strlen(argv[i]) + 1;
   }

#ifdef TRACE_SPAWN
   c_puts( ":: first 30 in strtbl before fork(): " );
   for( int i = 0; i < 30; ++i ) {
      char ch = strtbl[i];
      c_putchar( (ch >= ' ' && ch <= 0x7f) ? ch : '?' );
   }
   c_putchar( '\n' );
#endif


   // create the new process

   new = fork();
   if( new != 0 ) {   // error, or we are the parent
#if TRACE_SPAWN >= 3
      c_printf( ":: parent, fork() returned %d\n", new );
#endif
      return( new );
   }

   /*
   ** we are the child!
   */

   int myPid = pid();

   // set our priority
#if TRACE_SPAWN >= 3
   c_printf( ":: child, pid %d, setting priority to %d\n", myPid, prio );
#endif

   set_prio( myPid, prio );

   // create the argument vector

   char *args[MAX_ARGUMENTS+1];

#ifdef TRACE_SPAWN
   c_puts( ":: first 30 in strtbl in child: " );
   for( int i = 0; i < 30; ++i ) {
      char ch = strtbl[i];
      c_putchar( (ch >= ' ' && ch <= 0x7f) ? ch : '?' );
   }
   c_putchar( '\n' );
#endif

   ptr = strtbl;
#ifdef TRACE_SPAWN
   c_printf( ":: #%d creating argv; strtbl @ %08x", myPid, (uint32_t) ptr );
#endif
   for( int i = 0; i < argc; ++i ) {
#if TRACE_SPAWN >= 2
   c_printf( ", [%d] '%s'", i, ptr );
#endif
      args[i] = ptr;
      ptr += strlen(ptr) + 1;
   }

   args[argc] = NULL;

#ifdef TRACE_SPAWN
   c_putchar( '\n' );
   argdump( ":: args to exec(): ", (const char **) args, TRUE );
   c_putchar( '\n' );
#endif

   int num = exec( entry, args );

   // oops....

   new = pid();         // get our PID
   cvt_dec( buf, (int32_t) new );   // convert to character
   cwrites( "PID " );      // report the error
   cwrite( buf, strlen(buf) );
   cwrites( " exec() failed, status " );
   cvt_dec( buf, num );
   cwrite( buf, strlen(buf) );
   cwritech( '\n' );

   exit( EXIT_FAILURE );

   // keep the compiler happy!
   return( EXIT_FAILURE );
}
