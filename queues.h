/*
** SCCS ID:	@(#)queues.h	1.1	3/29/18
**
** File:	queues.h
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	Queue module declarations
*/

#ifndef _QUEUES_H_
#define _QUEUES_H_

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

// our queues (opaque to the system)

typedef struct st_queue *queue_t;

/*
** Globals
*/

extern queue_t _zombie;		// the living dead
extern queue_t _waiting;	// concerned parents
extern queue_t _sleeping;	// idlers

/*
** Prototypes
*/

/*
** _q_alloc()
**
** allocate a queue
**
** the single parameter will be used whenever queue insertions are done
*/
queue_t _q_alloc( int (*fcn)(const void *,const void*) );

/*
** _q_free(que)
**
** deallocate a queue
*/
void _q_free( queue_t que );

/*
** _q_empty(que)
**
** release any qnodes that might still be in a queue
*/
void _q_empty( queue_t que );

/*
** _q_size(que)
**
** return the current occupancy of a queue
*/
uint32_t _q_size( queue_t que );

/*
** _q_insert(que,data)
**
** insert a value into a queue
**
** the queue will be ordered by the comparison function supplied
** when it was created
*/
void _q_insert( queue_t que, void *data );

/*
** _q_peek(que)
**
** return the data field from the head node in the queue
*/
void *_q_peek( queue_t que );

/*
** _q_remove(que)
**
** remove the first entry from a queue
*/
void *_q_remove( queue_t que );

/*
** _q_remove_by(que,data,find)
**
** remove an entry from a queue
**
** this version removes an entry specified by a "find" function,
** which is invoked to determine whether or not the correct entry
** has been located.
*/
void *_q_remove_by( queue_t que, void *data,
                    int (*find)(const void*,const void*) );

/*
** _q_init()
**
** initialize the queue module
*/
void _q_init( void );

/*
** _q_prune()
**
** release any allocated space we aren't currently using
*/
uint32_t _q_prune( void );

/*
** _q_dump(msg,que)
**
** dump the contents of the specified queue to the console
*/
void _q_dump( const char *msg, queue_t que );

/*
** _q_dump_all(msg)
**
** dump the contents of all the queues to the console
*/
void _q_dump_all( const char *msg );

/*
** ORDERING FUNCTIONS
*/

/*
** _q_order_wakeup(a1,a2)
**
** used to order a queue by wakeup time
*/
int _q_order_wakeup( const void *a1, const void *a2 );

/*
** LOCATING FUNCTIONS
*/

/*
** _q_find_pid(data,proc)
**
** find a queue entry with the specified PID
**
** as PIDs are supposed to be unique, there should only be one in the queue
*/
int _q_find_pid( const void *data, const void *proc );

/*
** _q_find_ppid(data,proc)
**
** find a queue entry with the specified parent PID
**
** unlike PIDs, parent PIDs may not be unique in a queue, so we
** find the first entry having this PPID
*/
int _q_find_ppid( const void *data, const void *proc );

#endif

#endif

