/*
** SCCS ID:	@(#)ulib.h	1.1	3/29/18
**
** File:	ulib.h
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	declarations for user-level library functions
*/

#ifndef _ULIB_H_
#define _ULIB_H_

#include "common.h"

/*
** General (C and/or assembly) definitions
*/

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

/*
** Prototypes
*/

/*
**********************************************
** CONVENIENT "SHORTHAND" VERSIONS OF SYSCALLS
**********************************************
*/

/*
** creadch - read one character from the console
**
** usage:	n = creadch();
**
** reads one character from the console and return it
**
** returns:
**      the character read, or an error code
*/
int creadch( void );

/*
** cread - read into a buffer from the console
**
** usage:	n = cread(buf,size);
**
** reads up to 'size' characters from the console into 'buf'
**
** returns:
**      the count of characters read, or an error code
*/
int cread( char *buf, uint32_t size );

/*
** sreadch - read one character from the SIO
**
** usage:	n = sreadch();
**
** reads one character from the SIO and return it
**
** returns:
**      the character read, or an error code
*/
int sreadch( void );

/*
** sread - read into a buffer from the SIO
**
** usage:	n = sread(buf,size);
**
** reads up to 'size' characters from the SIO into 'buf'
**
** returns:
**      the count of characters read, or an error code
*/
int sread( char *buf, uint32_t size );

/*
** cwrites - write a NUL-terminated string to the console
**
** usage:	n = cwrites(str);
**
** writes a NUL-terminate string to the console
**
** returns:
**      the number of characters written, or an error code
*/
int cwrites( const char *str );

/*
** cwritech - write one character to the console
**
** usage:	n = cwritech(ch);
**
** writes one character to the console
**
** returns:
**      the number of characters written, or an error code
*/
int cwritech( char ch );

/*
** cwrite - write from a buffer to the console
**
** usage:	n = cread(buf,size);
**
** writes up to 'size' characters from 'buf' to the console
**
** returns:
**      the count of characters written, or an error code
*/
int cwrite( const char *buf, uint32_t size );

/*
** swritech - write one character to the SIO
**
** usage:	n = swritech(ch);
**
** writes one character to the SIO and return it
**
** returns:
**      the number of characters written, or an error code
*/
int swritech( char ch );

/*
** swrites - write a NUL-terminated string to the SIO
**
** usage:	n = swrites(str);
**
** writes a NUL-terminate string to the SIO
**
** returns:
**      the number of characters written, or an error code
*/
int swrites( const char *str );

/*
** swrite - write from a buffer to the SIO
**
** usage:	n = swrite(buf,size);
**
** writes up to 'size' characters from 'buf' to the SIO
**
** returns:
**      the count of characters written, or an error code
*/
int swrite( const char *buf, uint32_t size );

/*
** spawn - create a new process running a different program
**
** usage:       n = spawn(entry,args,prio)
**
** Creates a new process, changes its priority, and then execs 'entry'
**
** returns:
**      the PID of the child, or an error code
*/
int spawn(  int (*entry)(int,char *[]), char *argv[], uint8_t prio );

/*
**********************************************
** SYSTEM CALL STUBS
**********************************************
*/

/*
** exit - terminate the calling process
**
** usage:	exit(status);
**
** does not return
*/
void exit( int32_t status );

/*
** read - read into a buffer from a channel
**
** usage:	n = read(chan,buf,size)
**
** returns:
**	count of characters transferred, or an error code
*/
int read( int channel, char *buffer, uint32_t length );

/*
** write - write from a buffer to a channel
**
** usage:	n = write(chan,buf,size)
**
** returns:
**	count of characters transferred, or an error code
*/
int write( int channel, const char *buf, uint32_t length );

/*
** fork - create a new process
**
** usage:	pid = fork();
**
** returns:
**	parent - pid of the spawned process, or -1 on failure
**	child - 0
*/
int fork( void );

/*
** exec - replace this program with a different one
**
** usage:	pid = exec(entry,args);
**
** returns:
**	on success, does not return
**	on failure, an error code
*/
int exec( int (*entry)(int argc, char *argv[]), char *argv[] );

/*
** kill - terminate a process with extreme prejudice
**
** usage:       status = kill(pid);
**
** returns:
**      status of the termination attempt
*/
int32_t kill( uint16_t pid );

/*
** wait - wait for a child process to terminate
**
** usage:	pid = wait(&status);
**
** if there are no children in the system, returns an error code
**
** if there are one or more children in the system and at least one has
** terminated but hasn't yet been cleaned up, cleans up that process and
** returns its information; otherwise, blocks until a child terminates
**
** returns:
**      on error, an error code (*status is unchanged)
**      on success, the pid and exit status of the child
*/
int wait( int32_t *status );

/*
** sleep - put the current process to sleep for some length of time
**
** usage:	sleep(n);
**
** puts the process onto the sleep queue for the specified
** number of milliseconds
*/
void sleep( uint32_t msec );

/*
** pid - retrieve PID of this process
**
** usage:	n = pid();
**
** returns:
**      the PID of this process
*/
uint16_t pid( void );

/*
** ppid - retrieve PID of the parent of this process
**
** usage:	n = ppid();
**
** returns:
**      the PID of the parent of this process
*/
uint16_t ppid( void );

/*
** time - retrieve the current system time
**
** usage:	n = time();
**
** returns:
**      the current system time
*/
uint32_t time( void );

/*
** get_prio - retrieve the priority of the specified process
**
** usage:	prio = get_prio(pid);
**
** returns:
**      the priority of the process, or an error code
*/
int get_prio( uint16_t whom );

/*
** set_prio - set the priority of the specified process
**
** usage:	prio = set_prio(pid,prio);
**
** returns:
**      the old priority of the process, or an error code
*/
int set_prio( uint16_t whom, uint8_t prio );

/*
** bogus - a bogus system call, for testing our syscall ISR
**
** usage:	bogus();
*/
void bogus( void );

/*
**********************************************
** MISCELLANEOUS USEFUL SUPPORT FUNCTIONS
**********************************************
*/

/*
** report(ch,pid)
**
** report to the console that user 'ch' is running as 'pid'
*/
void report( char ch, uint16_t whom );

/*
** argdump(prefix,argv,mode)
**
** dump out a report of the user character and argv contents
*/
void argdump( const char *prefix, const char *argv[], int mode );

/*
** strlen()
**
** return length of a NUL-terminated string
*/
uint32_t strlen( const char *str );

/*
** strcpy - copy a NUL-terminated string
*/
char *strcpy( register char *dst, register const char *src );

/*
** strcat - append one string to another
*/
char *strcat( register char *dst, register const char *src );

/*
** strcmp - compare two NUL-terminated strings
*/
int strcmp( register const char *s1, register const char *s2 );

/*
** cvt_dec()
**
** convert a 32-bit integer value into a NUL-terminated character string
**
** returns the number of characters placed into the buffer
** (not including the NUL)
*/
int cvt_dec( char *buf, int32_t value );

/*
** cvt_hex()
**
** convert a 32-bit value into an (up to) 8-character
** NUL-terminated character string
**
** returns the number of characters placed into the buffer
** (not including the NUL)
*/
int cvt_hex( char *buf, uint32_t value );

/*
** do_exit()
**
** support function used as the "return to" address
** for user main() routines; it just calls exit(%eax)
*/
void do_exit( void );

/*
** get_ra()
**
** Get the return address for the calling function
** (i.e., where whoever called us will go back to)
*/
int get_ra( void );

#endif

#endif
