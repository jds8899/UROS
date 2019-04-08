/*
** SCCS ID:	@(#)users.h	1.1	3/29/18
**
** File:	users.h
**
** Author:	Warren R. Carithers and various CSCI-452 classes
**
** Contributor:
**
** Description:	control of user-level routines
*/

#ifndef _USER_H_
#define _USER_H_

/*
** General (C and/or assembly) definitions
*/

// delay loop counts

#define	DELAY_LONG	100000000
#define	DELAY_STD	  2500000
#define	DELAY_ALT	  4500000

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

/*
** User process controls.
**
** The comment field of these definitions contains a list of the
** system calls this user process uses.
**
** To spawn a specific user process from the initial process,
** uncomment its entry in this list.
*/

// syscalls in this system:  exit, read, write, fork, exec, kill, wait,
//                           sleep, pid, ppid, time, get_prio, set_prio

//	user	          baseline system calls in use
//		    xit rd  wrt frk exe kil wat sle pip Pid tim gtp stp
#define SPAWN_A	//   X   .   X   .   .   .   .   .   .   .   .   .   .
#define SPAWN_B	//   X   .   X   .   .   .   .   .   .   .   .   .   .
#define SPAWN_C	//   X   .   X   .   .   .   .   .   .   .   .   .   .
#define SPAWN_D	//   X   .   X   X   X   .   .   .   .   .   .   .   X
#define SPAWN_E	//   X   .   X   .   .   .   .   X   X   .   .   .   .
#define SPAWN_F	//   X   .   X   .   .   .   .   X   X   .   .   .   .
#define SPAWN_G	//   X   .   X   .   .   .   .   X   X   .   .   .   .
	// user H doesn't call exit()
#define SPAWN_H	//   .   .   X   .   .   .   .   .   .   .   .   .   .
// no user I
	// user J tries to spawn() 2*MAX_PROCS children
#define SPAWN_J	//   X   .   X   X   X   .   .   .   .   .   .   .   X
#define SPAWN_K	//   X   .   X   X   X   .   .   X   .   .   .   .   X
#define SPAWN_L	//   X   .   X   X   X   .   .   X   .   .   X   .   X
#define SPAWN_M	//   X   .   X   X   X   .   .   .   .   .   .   .   X
	// user N spawns W and Z at low priority
#define SPAWN_N	//   X   .   X   X   X   .   .   .   .   .   .   .   X
// no user O
#define SPAWN_P	//   X   .   X   .   .   .   .   X   .   .   X   .   .
 	// user Q makes a bogus system call
#define SPAWN_Q	//   .   .   X   .   .   .   .   .   .   .   .   .   .
#define SPAWN_R	//   X   X   X   .   .   .   .   X   .   .   .   .   .
	// user S loops forever
#define SPAWN_S	//   .   .   .   .   .   .   .   X   .   .   .   .   .
#define SPAWN_T	//   X   .   X   X   X   .   X   X   .   .   .   .   X
#define SPAWN_U	//   X   .   X   X   X   X   X   X   .   .   .   .   X
#if 0
#endif
// no user V
// users W-Z are spawned by other processes

/*
** Users W-Z are spawned from other processes; they
** should never be spawned directly by init().
*/

// user W:      //   X   .   X   .   .   .   .   X   X   .   .   .   .
	// user X exits with a non-zero status
// user X:      //   X   .   X   .   .   .   .   .   X   .   .   .   .
// user Y:      //   X   .   X   .   .   .   .   X   .   .   .   .   .
// user Z:      //   X   .   X   .   .   .   .   .   X   X   .   .   .


/*
** Prototypes for externally-visible routines
*/

/*
** init - initial user process
**
** after spawning the other user processes, loops forever calling wait()
*/
int init( int argc, char *argv[] );

/*
** idle - what to run when there's nothing else to run
*/
int idle( int argc, char *argv[] );

#endif

#endif
