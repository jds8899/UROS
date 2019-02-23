/*
** SCCS ID:	@(#)users.c	1.1	3/29/18
**
** File:	users.c
**
** Author:	Warren R. Carithers and various CSCI-452 classes
**
** Contributor:
**
** Description:	user routines
*/

#include "common.h"

#include "users.h"

/*
** USER PROCESSES
**
** Each is designed to test some facility of the OS; see the user.h
** header file for a summary of which system calls are tested by 
** each user function.
**
** Output from user processes is always alphabetic.  Uppercase 
** characters are "expected" output; lowercase are "erroneous"
** output.
**
** More specific information about each user process can be found in
** the header comment for that function (below).
**
** To spawn a specific user process, uncomment its SPAWN_x
** definition in the user.h header file.
*/

/*
** Prototypes for all one-letter user main routines (even
** ones that may not exist, for completeness)
*/

int user_a( int, char *[] ); int user_b( int, char *[] );
int user_c( int, char *[] ); int user_d( int, char *[] );
int user_e( int, char *[] ); int user_f( int, char *[] );
int user_g( int, char *[] ); int user_h( int, char *[] );
int user_i( int, char *[] ); int user_j( int, char *[] );
int user_k( int, char *[] ); int user_l( int, char *[] );
int user_m( int, char *[] ); int user_n( int, char *[] );
int user_o( int, char *[] ); int user_p( int, char *[] );
int user_q( int, char *[] ); int user_r( int, char *[] );
int user_s( int, char *[] ); int user_t( int, char *[] );
int user_u( int, char *[] ); int user_v( int, char *[] );
int user_w( int, char *[] ); int user_x( int, char *[] );
int user_y( int, char *[] ); int user_z( int, char *[] );

/*
** Users A, B, and C are identical, except for the character they
** print out via swritech().  Each prints its ID, then loops 30
** times delaying and printing, before exiting.  They also verify
** the status return from swrite().
*/

int user_a( int argc, char *argv[] ) {
   int i, j;
   int n;
   char buf[12];

   char ch = argc > 1 ? argv[1][0] : 'A';

   n = swritech( ch );
   if( n != 1 ) {
      cwrites( "User A, write 1 returned " );
      i = cvt_dec( buf, n );
      cwrite( buf, i );
      cwritech( '\n' );
   }
   for( i = 0; i < 30; ++i ) {
      for( j = 0; j < DELAY_STD; ++j )
         continue;
      n = swritech( ch );
      if( n != 1 ) {
         cwrites( "User A, write 2 returned " );
         i = cvt_dec( buf, n );
         cwrite( buf, i );
         cwritech( '\n' );
      }
   }

   exit( EXIT_SUCCESS );

   n = swrites( "*A*" );   /* shouldn't happen! */
   if( n != 3 ) {
      cwrites( "User A, write 3 returned " );
      i = cvt_dec( buf, n );
      cwrite( buf, i );
      cwritech( '\n' );
   }

   return( 0 );  // shut the compiler up!

}

int user_b( int argc, char *argv[] ) {
   int i, j;
   int n;
   char buf[12];

   char ch = argc > 1 ? argv[1][0] : 'B';

   n = swritech( ch );
   if( n != 1 ) {
      cwrites( "User B, write 1 returned " );
      i = cvt_dec( buf, n );
      cwrite( buf, i );
      cwritech( '\n' );
   }
   for( i = 0; i < 30; ++i ) {
      for( j = 0; j < DELAY_STD; ++j )
         continue;
      n = swritech( ch );
      if( n != 1 ) {
         cwrites( "User B, write 2 returned " );
         i = cvt_dec( buf, n );
         cwrite( buf, i );
         cwritech( '\n' );
      }
   }

   exit( EXIT_SUCCESS );

   n = swritech( 'b' );   /* shouldn't happen! */
   if( n != 1 ) {
      cwrites( "User B, write 3 returned " );
      i = cvt_dec( buf, n );
      cwrite( buf, i );
      cwritech( '\n' );
   }
   return( 0 );  // shut the compiler up!

}

int user_c( int argc, char *argv[] ) {
   int i, j;
   int n;
   char buf[12];

   char ch = argc > 1 ? argv[1][0] : 'C';

   n = swritech( ch );
   if( n != 1 ) {
      cwrites( "User C, write 1 returned " );
      i = cvt_dec( buf, n );
      cwrite( buf, i );
      cwritech( '\n' );
   }
   for( i = 0; i < 30; ++i ) {
      for( j = 0; j < DELAY_STD; ++j )
         continue;
      n = swritech( ch );
      if( n != 1 ) {
         cwrites( "User C, write 2 returned " );
         i = cvt_dec( buf, n );
         cwrite( buf, i );
         cwritech( '\n' );
      }
   }

   exit( EXIT_SUCCESS );

   n = swritech( 'c' );
   if( n != 1 ) {
      cwrites( "User C, write 3 returned " );
      i = cvt_dec( buf, n );
      cwrite( buf, i );
      cwritech( '\n' );
   }
   return( 0 );  // shut the compiler up!

}


/*
** User D spawns user Z, then exits before it can terminate.
*/

int user_d( int argc, char *argv[] ) {
   uint16_t whom;
   int ret = EXIT_SUCCESS;
   char *args[] = { NULL, NULL, NULL };

   char ch = argc > 1 ? argv[1][0] : 'D';

   swritech( ch );

   args[0] = "user_z";
   args[1] = "^";

   whom = spawn( user_z, args, PRIO_USER_HIGH );
   if( whom < 0 ) {
      cwrites( "User D spawn() failed\n" );
      ret = EXIT_FAILURE;
   }

   swritech( ch );

   exit( ret );

   return( 0 );  // shut the compiler up!

}


/*
** Users E, F, and G test the sleep facility.
**
** User E sleeps for 10 seconds at a time.
*/

int user_e( int argc, char *argv[] ) {

   char ch = argc > 1 ? argv[1][0] : 'E';

   report( 'E', pid() );
   swritech( ch );
   for( int i = 0; i < 5 ; ++i ) {
      sleep( SECONDS_TO_MS(10) );
      swritech( ch );
   }

   exit( EXIT_SUCCESS );
   return( 0 );  // shut the compiler up!

}

/*
** User F sleeps for 5 seconds at a time.
*/

int user_f( int argc, char *argv[] ) {

   char ch = argc > 1 ? argv[1][0] : 'F';

   report( 'F', pid() );
   swritech( ch );
   for( int i = 0; i < 5 ; ++i ) {
      sleep( SECONDS_TO_MS(5) );
      swritech( ch );
   }

   exit( EXIT_SUCCESS );
   return( 0 );  // shut the compiler up!

}

/*
** User G sleeps for 15 seconds at a time.
*/

int user_g( int argc, char *argv[] ) {

   char ch = argc > 1 ? argv[1][0] : 'G';

   report( 'G', pid() );
   swritech( ch );
   for( int i = 0; i < 5 ; ++i ) {
      sleep( SECONDS_TO_MS(15) );
      swritech( ch );
   }

   exit( EXIT_SUCCESS );
   return( 0 );  // shut the compiler up!

}


/*
** User H is like A-C except it only loops 5 times and doesn't
** call exit().
*/

int user_h( int argc, char *argv[] ) {

   char ch = argc > 1 ? argv[1][0] : 'H';

   swritech( ch );
   for( int i = 0; i < 5; ++i ) {
      for( int j = 0; j < DELAY_STD; ++j )
         continue;
      swritech( ch );
   }

   cwrites( "User H returning without exiting!\n" );

   return( 0 );

}


/*
** User J tries to spawn 2*MAX_PROCS copies of user_y.
*/

int user_j( int argc, char *argv[] ) {
   char *args[] = { NULL, NULL, NULL };
   char num[32];  // converted sequence number

   char ch = argc > 1 ? argv[1][0] : 'J';

   swritech( ch );

   args[0] = "user_y";
   args[1] = num;

   for( int i = 0; i < (MAX_PROCS * 2) ; ++i ) {
      (void) cvt_dec( num, i );
      uint16_t whom = spawn( user_y, args, PRIO_USER_STD );
      if( whom < 0 ) {
         swritech( 'j' );
      } else {
         swritech( ch );
      }
   }

   exit( EXIT_SUCCESS );

   return( 0 );  // shut the compiler up!

}

/*
** User K prints, goes into a loop which runs five times, and exits.
** In the loop, it does a spawn of user_x, sleeps 30 seconds, and prints.
** It gives user_x() numbers in the 0..4 range.
*/

int user_k( int argc, char *argv[] ) {
   char *args[] = { NULL, NULL, NULL };
   char num[32];  // converted sequence number

   char ch = argc > 1 ? argv[1][0] : 'K';

   swritech( ch );

   args[0] = "user_x";
   args[1] = num;

   for( int i = 0; i < 5 ; ++i ) {
      (void) cvt_dec( num, i );
      swritech( ch );
      uint16_t whom = spawn( user_x, args, PRIO_USER_STD );
      if( whom < 0 ) {
         swritech( 'k' );
      }
      sleep( SECONDS_TO_MS(30) );
   }

   exit( EXIT_SUCCESS );

   return( 0 );  // shut the compiler up!

}


/*
** User L is like user K, except that it prints times and doesn't sleep
** each time, it just preempts itself.  It also gives user_x() numbers
** in the 100..104 range.
*/

int user_l( int argc, char *argv[] ) {
   int i;
   uint32_t now;
   char buf[8];
   char *args[] = { NULL, NULL, NULL };
   char num[32];  // converted sequence number

   char ch = argc > 1 ? argv[1][0] : 'L';

   args[0] = "user_x";
   args[1] = num;

   now = time();
   cwrites( "User L running, initial time " );
   i = cvt_hex( buf, now );
   cwrite( buf, i );
   cwritech( '\n' );

   swritech( ch );

   for( i = 0; i < 5 ; ++i ) {
      (void) cvt_dec( num, i + 100 );
      swritech( ch );
      uint16_t whom = spawn( user_x, args, PRIO_USER_STD );
      if( whom < 0 ) {
         swritech( 'l' );
      } else {
         // yield, but don't sleep
         sleep( 0 );
      }
   }

   now = time();
   cwrites( "User L exiting, time " );
   i = cvt_hex( buf, now );
   cwrite( buf, i );
   cwritech( '\n' );

   exit( EXIT_SUCCESS );

   return( 0 );  // shut the compiler up!

}


/*
** User M iterates spawns five copies of user W, reporting their PIDs.
*/

int user_m( int argc, char *argv[] ) {
   int i;
   char buf[12];
   char *args[] = { NULL, NULL, NULL };
   char num[32];

   char ch = argc > 1 ? argv[1][0] : 'M';

   args[0] = "user_w";
   args[1] = num;

   swritech( ch );

   for( i = 0; i < 5; ++i ) {
      (void) cvt_dec( num, i + 10 );
      swritech( ch );
      uint16_t whom = spawn( user_w, args, PRIO_USER_HIGH );
      if( whom < 0 ) {
         swritech( 'm' );
      } else {
         cwrites( "M spawned W, PID " );
         i = cvt_dec( buf, whom );
         cwrite( buf, i );
         cwritech( '\n' );
      }
   }

   exit( EXIT_SUCCESS );
   return( 0 );  // shut the compiler up!

}


/*
** User N is like user M, except that it spawns user W and user Z
** at lower priority
*/

int user_n( int argc, char *argv[] ) {
   int i;
   uint16_t whom;
   char buf[12];
   char *args[] = { NULL, NULL, NULL };
   char num[32];

   char ch = argc > 1 ? argv[1][0] : 'N';

   args[1] = num;

   swritech( ch );

   for( i = 0; i < 5; ++i ) {
      (void) cvt_dec( num, i + 1000 );
      args[0] = "user_w";
      swritech( ch );
      whom = spawn( user_w, args, PRIO_USER_STD );
      if( whom < 0 ) {
         swritech( 'n' );
      } else {
         cwrites( "User N spawned W, PID " );
         i = cvt_dec( buf, whom );
         cwrite( buf, i );
         cwritech( '\n' );
      }

      args[0] = "user_z";
      whom = spawn( user_z, args, PRIO_USER_STD );
      if( whom < 0 ) {
         swritech( 'n' );
      } else {
         cwrites( "User N spawned Z, PID " );
         i = cvt_dec( buf, whom );
         cwrite( buf, i );
         cwritech( '\n' );
      }
   }

   exit( EXIT_SUCCESS );

   return( 0 );  // shut the compiler up!

}


/*
** User P iterates three times.  Each iteration sleeps for two seconds,
** then gets and prints the system time.
*/

int user_p( int argc, char *argv[] ) {
   uint32_t now;
   int i;
   char buf[8];

   char ch = argc > 1 ? argv[1][0] : 'P';

   cwrites( "User P running, start at " );
   now = time();
   i = cvt_hex( buf, now );
   cwrite( buf, i );
   cwritech( '\n' );

   swritech( ch );

   for( i = 0; i < 3; ++i ) {
      sleep( SECONDS_TO_MS(2) );
      now = time();
      cwrites( "User P reporting time " );
      i = cvt_hex( buf, now );
      cwrite( buf, i );
      cwritech( '\n' );
      swritech( ch );
   }

   exit( EXIT_SUCCESS );

   return( 0 );  // shut the compiler up!

}


/*
** User Q does a bogus system call
*/

int user_q( int argc, char *argv[] ) {

   char ch = argc > 1 ? argv[1][0] : 'Q';

   swritech( ch );
   bogus();
   cwrites( "User Q returned from bogus syscall!?!?!\n" );
   exit( EXIT_FAILURE );

   return( 0 );  // shut the compiler up!

}


/*
** User R loops 3 times reading/writing, then exits.
*/

int user_r( int argc, char *argv[] ) {
   char buf[12];

   char ch = argc > 1 ? argv[1][0] : 'R';

   swritech( ch );
   sleep( SECONDS_TO_MS(10) );
   for( int i = 0; i < 3; ++i ) {
      do {
         swritech( ch );
         ch = sreadch();
         if( ch < 0 ) {
            cwrites( "User R, read returned " );
            int j = cvt_dec( buf, ch );
            cwrite( buf, j );
            cwritech( '\n' );
            if( ch == -1 ) {
               // wait a bit
               sleep( SECONDS_TO_MS(1) );
            }
         }
      } while( ch < 0 );
      swritech( ch );
   }

   exit( EXIT_SUCCESS );

   return( 0 );  // shut the compiler up!

}


/*
** User S sleeps for 20 seconds at a time, and loops forever.
*/

int user_s( int argc, char *argv[] ) {

   char ch = argc > 1 ? argv[1][0] : 'S';

   swritech( ch );
   for(;;) {
      sleep( SECONDS_TO_MS(20) );
      swritech( ch );
   }

   cwrites( "User S exiting!?!?!n" );
   swritech( 's' );
   exit( EXIT_FAILURE );

   return( 0 );  // shut the compiler up!

}

/*
** User T iterates three times, spawning copies of user W; then it
** sleeps for eight seconds, and then waits for those processes.
*/

int user_t( int argc, char *argv[] ) {
   uint16_t whom[3];
   char buf[12];
   char *args[] = { NULL, NULL, NULL };
   char num[32];

   char ch = argc > 1 ? argv[1][0] : 'T';

   args[0] = "user_w";
   args[1] = num;

   swritech( ch );

   for( int i = 0; i < 3; ++i ) {
      (void) cvt_dec( num, i + 20 );
      whom[i] = spawn( user_w, args, PRIO_USER_STD );
      if( whom[i] < 0 ) {
         swritech( 't' );
      } else {
         swritech( ch );
      }
   }

   sleep( SECONDS_TO_MS(8) );

   // collect exit status information

   do {
      uint16_t this;
      int32_t exitstatus;
      this = wait( &exitstatus );
      if( this == E_NO_KIDS ) {
         break;
      } else if( this != E_SUCCESS ) {
         cwrites( "User T: wait() status " );
         int n = cvt_dec( buf, this );
         cwrite( buf, n );
         cwritech( '\n' );
         break;
      }
      cwrites( "User T: child PID " );
      int j = cvt_dec( buf, this );
      cwrite( buf, j );
      cwrites( " exit status " );
      int n = cvt_dec( buf, exitstatus );
      cwrite( buf, n );
      cwritech( '\n' );
   } while( 1 );

   exit( EXIT_SUCCESS );
   return( 0 );  // shut the compiler up!

}


/*
** User U is like user T, except that it kills any living children
** after sleeping.
*/

int user_u( int argc, char *argv[] ) {
   int whom[3];
   int32_t status;
   char buf[12];
   char *args[] = { NULL, NULL, NULL };
   char num[32];

   char ch = argc > 1 ? argv[1][0] : 'U';

   args[0] = "user_w";
   args[1] = num;

   swritech( ch );

   for( int i = 0; i < 3; ++i ) {
      (void) cvt_dec( num, i + 30 );
      whom[i] = spawn( user_w, args, PRIO_USER_HIGH );
      if( whom[i] < 0 ) {
         swritech( 'u' );
      } else {
         cwrites( "User U spawned W, PID " );
         int j = cvt_dec( buf, whom[i] );
         cwrite( buf, j );
         cwritech( '\n' );
         swrite( "U", 1 );
      }
   }

   sleep( SECONDS_TO_MS(8) );

   for( int i = 0; i < 3; ++i ) {
      if( whom[i] > 0 ) {
         status = kill( whom[i] );
         cwrites( "User U kill PID " );
         int j = cvt_dec( buf, whom[i] );
         cwrite( buf, j );
         cwrites( " status " );
         j = cvt_dec( buf, status );
         cwrite( buf, j );
         cwritech( '\n' );
      }
   }

   // collect exit status information

   do {
      uint16_t this;
      int32_t exitstatus;
      this = wait( &exitstatus );
      if( this == E_NO_KIDS ) {
         break;
      } else if( this != E_SUCCESS ) {
         cwrites( "User U: wait() status " );
         int n = cvt_dec( buf, this );
         cwrite( buf, n );
         cwritech( '\n' );
         break;
      }
      cwrites( "User U: child " );
      int j = cvt_dec( buf, this );
      cwrite( buf, j );
      cwrites( " exit status " );
      j = cvt_dec( buf, exitstatus );
      cwrite( buf, j );
      cwritech( '\n' );
   } while( 1 );

   exit( EXIT_SUCCESS );
   return( 0 );  // shut the compiler up!

}

/*
** Users W through Z are spawned by other user processes.  Each
** prints the entire argv[1] string (not just a single character)
** to the SIO on each iteration.
*/


/*
** User W prints W characters 20 times, sleeping 3 seconds between.
*/

int user_w( int argc, char *argv[] ) {

   char *msg = argc > 1 ? argv[1] : "W'?'";

   report( 'W', pid() );

   for( int i = 0; i < 20 ; ++i ) {
      swrites( msg );
      sleep( SECONDS_TO_MS(3) );
   }

   cwrites( "User " );
   cwrites( msg );
   cwrites( " exiting\n" );

   exit( EXIT_SUCCESS );
   return( 0 );  // shut the compiler up!

}


/*
** User X prints X characters 20 times.  It is spawned multiple
** times, prints its PID when started and exiting, and exits
** with a non-zero status
*/

int user_x( int argc, char *argv[] ) {
   int i, j;
   uint16_t whom;

   char *msg = argc > 1 ? argv[1] : "X'?'";

   report( 'X', whom=pid() );

   for( i = 0; i < 20 ; ++i ) {
      swrites( msg );
      for( j = 0; j < DELAY_STD; ++j )
         continue;
   }

   cwrites( "User " );
   cwrites( msg );
   cwrites( " exiting\n" );
   exit( whom ? whom : -98765 );

   return( 0 );  // shut the compiler up!

}


/*
** User Y prints Y characters 10 times.
*/

int user_y( int argc, char *argv[] ) {
   int i, j;

   char *msg = argc > 1 ? argv[1] : "Y'?'";

   for( i = 0; i < 10 ; ++i ) {
      swrites( msg );
      for( j = 0; j < DELAY_ALT; ++j )
         continue;
      sleep( SECONDS_TO_MS(1) );
   }

   exit( EXIT_SUCCESS );

   return( 0 );  // shut the compiler up!

}


/*
** User Z prints Z characters 10 times.  Before it exits, its parent
** may have exited; it reports this so that we can verify reparenting.
*/

int user_z( int argc, char *argv[] ) {
   int i, j;
   uint16_t me, parent;
   char buf[12];

   char *msg = argc > 1 ? argv[1] : "Z'?'";

   me = pid();
   parent = ppid();

   cwrites( "User " );
   cwrites( msg );
   cwrites( "(" );
   i = cvt_dec( buf, me );
   cwrite( buf, i );
   cwrites( ") running, parent " );
   i = cvt_dec( buf, parent );
   cwrite( buf, i );
   cwritech( '\n' );

   for( i = 0; i < 10 ; ++i ) {
      swrites( msg );
      for( j = 0; j < DELAY_STD; ++j )
         continue;
   }

   // get "new" parent PID
   parent = ppid();

   cwrites( "User " );
   cwrites( msg );
   cwrites( "(" );
   i = cvt_dec( buf, me );
   cwrite( buf, i );
   cwrites( ") exiting, parent now " );
   i = cvt_dec( buf, parent );
   cwrite( buf, i );
   cwritech( '\n' );

   exit( EXIT_SUCCESS );

   return( 0 );  // shut the compiler up!

}


/*
** SYSTEM PROCESSES
*/

/*
** Idle process
**
** configurable - uses the argument as the character to print
*/

int idle( int argc, char *argv[] ) {
   int i;
   uint16_t me;
   uint32_t now;
   char buf[12];

   char ch = argc > 1 ? argv[1][0] : '*';
   
   me = pid();
   now = time();
   cwrites( "Idle (" );
   i = cvt_dec( buf, me );
   cwrite( buf, i );
   cwrites( ") started @ " );
   i = cvt_hex( buf, now );
   cwrite( buf, i );
   cwritech( '\n' );

   swritech( ch );

   for(;;) {
      for( i = 0; i < DELAY_LONG; ++i )
         continue;
      swritech( ch );
   }

   now = time();
   cwrites( "+++ Idle done @ " );
   i = cvt_hex( buf, now );
   cwrite( buf, i );
   cwrites( "!?!?!\n" );

   exit( EXIT_FAILURE );
   return( 0 );  // shut the compiler up!

}


/*
** Initial process; it starts the other top-level user processes.
**
** Prints '$' at startup, '+' after each user process is spawned,
** and '!' before transitioning to wait() mode to the SIO, and
** startup and transition messages to the console.  It also reports
** each child process it collects via wait() to the console along
** with that child's exit status.
*/

int init( int argc, char *argv[] ) {
   int whom;
   char *args[3], arg0[24], arg1[24];
   static int invoked = 0;

   args[0] = arg0; args[1] = arg1; args[2] = NULL;

   if( invoked > 0 ) {
      cwrites( "Init RESTARTED???\n" );
      for(;;);
   }

   cwrites( "Init started\n" );
   ++invoked;

   // home up, clear
   swrites( "\x1a" );
   // wait a bit
   for( int i = 0; i < DELAY_STD; ++i )
      ;
   swrites( "\n\nSpem relinquunt qui huc intrasti!\n\n\r" );

   args[0] = "idle"; args[1] = ".";
   whom = spawn( idle, args, PRIO_LOWEST );
   if( whom < 0 ) {
      cwrites( "init, spawn() IDLE failed\n" );
   }
   swritech( '+' );

#ifdef SPAWN_A
   args[0] = "user_a"; args[1] = "A";
   whom = spawn( user_a, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user A failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_B
   args[0] = "user_b"; args[1] = "B";
   whom = spawn( user_b, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user B failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_C
   args[0] = "user_c"; args[1] = "C";
   whom = spawn( user_c, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user C failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_D
   args[0] = "user_d"; args[1] = "D";
   whom = spawn( user_d, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user D failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_E
   args[0] = "user_e"; args[1] = "E";
   whom = spawn( user_e, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user E failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_F
   args[0] = "user_f"; args[1] = "F";
   whom = spawn( user_f, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user F failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_G
   args[0] = "user_g"; args[1] = "G";
   whom = spawn( user_g, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user G failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_H
   args[0] = "user_h"; args[1] = "H";
   whom = spawn( user_h, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user H failed\n" );
   }
   swritech( '+' );
#endif

   // there is no user_i

#ifdef SPAWN_J
   args[0] = "user_j"; args[1] = "J";
   whom = spawn( user_j, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user J failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_K
   args[0] = "user_k"; args[1] = "K";
   whom = spawn( user_k, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user K failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_L
   args[0] = "user_l"; args[1] = "L";
   whom = spawn( user_l, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user L failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_M
   args[0] = "user_m"; args[1] = "M";
   whom = spawn( user_m, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user M failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_N
   args[0] = "user_n"; args[1] = "N";
   whom = spawn( user_n, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user N failed\n" );
   }
   swritech( '+' );
#endif

   // there is no user_o

#ifdef SPAWN_P
   args[0] = "user_p"; args[1] = "P";
   whom = spawn( user_p, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user P failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_Q
   args[0] = "user_q"; args[1] = "Q";
   whom = spawn( user_q, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user Q failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_R
   args[0] = "user_r"; args[1] = "R";
   whom = spawn( user_r, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user R failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_S
   args[0] = "user_s"; args[1] = "S";
   whom = spawn( user_s, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user S failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_T
   args[0] = "user_t"; args[1] = "T";
   whom = spawn( user_t, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user T failed\n" );
   }
   swritech( '+' );
#endif

#ifdef SPAWN_U
   args[0] = "user_u"; args[1] = "U";
   whom = spawn( user_u, args, PRIO_USER_STD );
   if( whom < 0 ) {
      cwrites( "init, spawn() user U failed\n" );
   }
   swritech( '+' );
#endif

   // there is no user_v
   // users w through z are spawned elsewhere

   swrites( "!\n\n" );
   // cwrites( "!\n" );

   /*
   ** At this point, we go into an infinite loop waiting
   ** for our children (direct, or inherited) to exit.
   */

   cwrites( "init() transitioning to wait() mode\n" );

   for(;;) {
      int i;
      int32_t exitstatus;
      char buf[12];
      uint16_t whom = wait( &exitstatus );
      if( whom == E_NO_KIDS ) {
         continue;
      } else if( whom > 0 ) {
         cwrites( "INIT: pid " );
         i = cvt_dec( buf, whom );
         cwrite( buf, i );
         cwrites( " exited, status " );
         i = cvt_dec( buf, exitstatus );
         cwrite( buf, i );
         cwritech( '\n' );
      } else {
         cwrites( "*** init(): wait() status " );
         i = cvt_dec( buf, whom );
         cwrite( buf, i );
         cwritech( '\n' );
      }
   }

   /*
   ** SHOULD NEVER REACH HERE
   */

   cwrites( "*** INIT IS EXITING???\n" );
   exit( EXIT_FAILURE );

   return( 0 );  // shut the compiler up!
}
