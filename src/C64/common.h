/*
** SCCS ID:	@(#)common.h	1.1	3/29/18
**
** File:	common.h
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	Declarations used by both kernel and user code
*/

#ifndef _COMMON_H_
#define _COMMON_H_

/*
** General (C and/or assembly) definitions
**
** These are used by both kernel and user code
*/

// maximum number of simultaneous processes the system will support

#define	MAX_PROCS	25

// maximum number of characters passable in argv

#define	MAX_ARGV_CHARS	512

// maximum number of arguments passable in argv

#define	MAX_ARGUMENTS	64

// i/o channels

#define	CHAN_CONSOLE	0
#define	CHAN_SIO	1

// predefined exit status values

#define	EXIT_SUCCESS	0
#define	EXIT_FAILURE	1
#define	EXIT_KILLED	2
#define	EXIT_BAD_CODE	3

// some pseudo-Booleans

#define	TRUE		1
#define	FALSE		0

#ifndef __SP_ASM__

// only do these in if we're not in assembly language

/*
** Start of C-only definitions
*/

// clock interrupts per second

#define	CLOCK_FREQUENCY		1000

#define	TICKS_PER_MS		1

// pseudo functions to convert between seconds, ms, and ticks

#define	SECONDS_TO_MS(n)	((n) * 1000)

#define	MS_TO_TICKS(n)		(n)

#define	SECONDS_TO_TICKS(n)	(MS_TO_TICKS(SECONDS_TO_MS(n)))

#define	TICKS_TO_SECONDS(n)	((n) / CLOCK_FREQUENCY)

#define	TICKS_TO_ROUNDED_SECONDS(n)	(((n)+(CLOCK_FREQUENCY-1)) / CLOCK_FREQUENCY)

// error codes

#define	E_NO_ERROR		(0)
#define	E_SUCCESS		E_NO_ERROR

#define	E_BAD_CHAN		(-1)
#define	E_BAD_PID		(-2)
#define	E_BAD_PRIO		(-3)
#define	E_EOF			(-4)
#define	E_IO_ERROR		(-5)
#define	E_NO_KIDS		(-6)
#define	E_NO_PCBS		(-7)
#define	E_NO_STACKS		(-8)
#define	E_TOO_MANY_PROCS	(-9)
#define	E_UNDEAD		(-10)
#define	E_NO_DATA		(-11)
#define	E_NO_MORE_PROCS		(-12)
#define	E_BAD_ARGS		(-13)
#define	E_TOO_MANY_ARGS		(-14)
#define	E_TOO_MANY_ARG_CHARS	(-15)

// correct (?) way to define NULL

#define	NULL	((void *) 0)

// process priorities (here because users need to know them)

enum e_prios {
   PRIO_SYS_HIGH = 0, PRIO_SYS_STD = 1,
   PRIO_USER_HIGH = 2, PRIO_USER_STD = 3,
   PRIO_LOWEST = 4
};

// minimum and maximum priorities, by internal representation

#define	PRIO_MIN	PRIO_SYS_HIGH
#define	PRIO_MAX	PRIO_LOWEST

#define	N_PRIOS		(PRIO_MAX + 1)

// quick check for validity of a priority value

#define	VALID_PRIO(n)	((n) >= PRIO_MIN && (n) <= PRIO_MAX)

/*
** Types
*/

// size-specific integer types

typedef char			int8_t;
typedef unsigned char		uint8_t;

typedef short			int16_t;
typedef unsigned short		uint16_t;

typedef int			int32_t;
typedef unsigned int		uint32_t;

typedef long long int		int64_t;
typedef unsigned long long int	uint64_t;

#ifdef __SP_KERNEL__

// OS needs the system headers and the kernel library headers

#include "c_io.h"
#include "kmalloc.h"
#include "support.h"
#include "system.h"

#include "klib.h"

#else

// User code needs only the user library headers

#include "ulib.h"

#endif

/*
** Globals
*/

/*
** Prototypes
*/

#endif

#endif
