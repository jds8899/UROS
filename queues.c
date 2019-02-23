/*
** SCCS ID:	@(#)queues.c	1.1	3/29/18
**
** File:	queues.c
**
** Author:	CSCI-452 class of 20175
**
** Contributor:
**
** Description:	Queue module implementations
*/

#define	__SP_KERNEL__

#include "common.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** Our queues are generic, self-ordering queues.  To keep track of
** the contents of each queue, we use "queue nodes" to create a
** singly-linked list of entries.
**
** All of the internal workings of the queues are hidden from the
** rest of the system.
*/

// queue nodes

typedef struct st_qnode {
   void *data;
   struct st_qnode *next;
} qnode_t;

/*
** The queues themselves are fairly simple, with just head and tail
** pointers, an occupancy count, and a pointer to the comparison
** function to be used for ordering.
*/

struct st_queue {
   qnode_t *head;	// first entry in queue
   qnode_t *tail;	// last entry in queue
   uint32_t size;	// occupancy of the queue
   int (*compare)(const void *,const void *);	// comparison function
};

// include this here to get the queue_t type definition

#include "queues.h"

// for _ready
#include "scheduler.h"

// for _reading
#include "sio.h"

/*
** PRIVATE GLOBAL VARIABLES
*/

static qnode_t *_free_qnodes;

/*
** PUBLIC GLOBAL VARIABLES
*/

queue_t _zombie;	// the living dead
queue_t _waiting;	// concerned parents
queue_t _sleeping;	// idlers

/*
** PRIVATE FUNCTIONS
*/

/*
** _qnode_alloc()
**
** allocate a qnode
**
** re-uses a previously deallocated node if possible; otherwise,
** dynamically allocates a new one.
*/

static qnode_t *_qnode_alloc( void ) {
   qnode_t *new;
   
   // see if we can re-use one that already exists

   if( _free_qnodes ) {

      // yes!  take the first one
      new = _free_qnodes;
      _free_qnodes = new->next;

   } else {

      // nope - try to create one from thin air
      try_again:
      new = _kmalloc( sizeof(qnode_t) );

   }
   
   // verify that we got one

   if( new == NULL ) {

      // no - see if we can free up some memory

      if( !_prune() ) {
         _kpanic( "_qnode_alloc", "alloc failed" );
      }

      // yes, we could; give it another shot.
      goto try_again;
   }
   
   // got one!  clear the fields, and return it to the caller

   new->data = NULL;
   new->next = NULL;
   return( new );
}

/*
** _qnode_free(qnode)
**
** deallocate a qnode
**
** returns the qnode to the free list
*/

static void _qnode_free( qnode_t *node ) {

#if defined(SANITY_CHECK)
   // verify that we were given a valid parameter

   if( node == NULL ) {
      _kpanic( "_qnode_free", "NULL argument" );
   }
#endif

   // disconnect this qnode from any data it might have contained

   node->data = NULL;

   // put it at the beginning of the free list

   node->next = _free_qnodes;
   _free_qnodes = node;
}

/*
** PUBLIC FUNCTIONS
*/

/*
** _q_alloc()
**
** allocate a queue
**
** the single parameter will be used whenever queue insertions are done
*/

queue_t _q_alloc( int (*fcn)(const void *,const void*) ) {
   queue_t new;
   
   // allocate a queue structure

   new = _kmalloc( sizeof(struct st_queue) );

   // if it worked, initialize it

   if( new != NULL ) {
      new->head = NULL;
      new->tail = NULL;
      new->size = 0;
      new->compare = fcn;
   }
   
   // return whatever we got (valid pointer, or NULL)

   return( new );
}


/*
** _q_free(que)
**
** deallocate a queue
*/

void _q_free( queue_t que ) {

#if defined(SANITY_CHECK)
   // verify that we got an actual pointer

   if( que == NULL ) {
      _kpanic( "_q_free", "NULL argument" );
   }
#endif

   // check to be sure that the queue is empty

   if( _q_size(que) > 0 ) {
      _q_empty( que );
   }
   
   // put it back in the wild

   _kfree( que );
}

/*
** _q_empty(que)
**
** release any qnodes that might still be in a queue
*/

void _q_empty( queue_t que ) {
   qnode_t *curr, *tmp;

#if defined(SANITY_CHECK)
   // verify that we got an actual pointer

   if( que == NULL ) {
      _kpanic( "_q_free", "NULL argument" );
   }
#endif
   
   // follow the list of qnode pointers, releasing them as we go

   curr = que->head;
   while( curr != NULL ) {
      tmp = curr;
      curr = tmp->next;
      _qnode_free( tmp );
   }
   
   // reset the queue data fields

   que->head = que->tail = NULL;
   que->size = 0;
}

/*
** _q_size(que)
**
** return the current occupancy of a queue
*/

uint32_t _q_size( queue_t que ) {

   return( que->size );
}

/*
** _q_insert(que,data)
**
** insert a value into a queue
**
** the queue will be ordered by the comparison function supplied
** when it was created
*/

void _q_insert( queue_t que, void *data ) {
   int (*cmp)(const void *,const void *);
   
#if defined(SANITY_CHECK)
   // verify that we got a valid queue

   if( que == NULL ) {
      _kpanic( "_q_insert", "NULL queue parameter" );
   }
#endif

   // allocate a queue node, and fill in its data field

   qnode_t *new;
   new = _qnode_alloc();

   new->data = data;
   
   // if the queue is empty, just add the node

   if( _q_size(que) == 0 ) {
      que->head = que->tail = new;
      que->size = 1;
      return;
   }
   
   // the queue isn't empty - do we need to do
   // an ordered insertion?

   if( que->compare == NULL ) {
      // no - just add at the end
      que->tail->next = new;
      que->tail = new;
      que->size += 1;
      return;
   }
   
   // we need to do an ordered insertion

   cmp = que->compare;
   qnode_t *prev, *curr;
   
   // iterate through the queue contents, looking for the
   // correct insertion point

   prev = NULL;
   curr = que->head;
   
   while( curr && cmp(curr->data,data) <= 0 ) {
      prev = curr;
      curr = curr->next;
   }
   
   /*
   **  prev  curr  todo
   **  ====  ====  ===================
   **  NULL  NULL  panic
   **  NULL  ----  add at front: new --> curr, head --> new
   **  ----  ----  add middle: new --> curr, prev --> new
   **  ----  NULL  add at end:  new --> curr, prev --> new, tail --> new
   */
   
   // always make the new node point to its successor

   new->next = curr;
   
   // are we adding at the beginning?

   if( prev == NULL ) {

      // yes - make sure there isn't a problem with the queue

      if( curr == NULL ) {
         _kpanic( "_q_insert", "empty non-empty queue???" );
      }

      // this node already points to the "old" first node

      que->head = new;

   } else {

      // inserting in the middle, or at the end

      prev->next = new;

      if( curr == NULL ) {
         // this is the new "tail" of the queue
         que->tail = new;
      }
   }
   
   // one more occupant!

   que->size += 1;
}


/*
** _q_peek(que)
**
** return the data field from the head node in the queue
*/

void *_q_peek( queue_t que ) {

#if defined(SANITY_CHECK)
   // verify that we got an actual pointer

   if( que == NULL ) {
      _kpanic( "_q_peek", "NULL argument" );
   }
#endif

   // if there's nothing here, just return NULL
   //
   // NOTE: caller must determine whether NULL
   // means "empty" or "first data value is 0"

   if( _q_size(que) == 0 ) {
      return( NULL );
   }
   
   // send back the first data field

   return( que->head->data );
}

/*
** _q_remove(que)
**
** remove the first entry from a queue
*/

void *_q_remove( queue_t que ) {

#if defined(SANITY_CHECK)
   // verify that we got an actual pointer

   if( que == NULL ) {
      _kpanic( "_q_remove", "NULL argument" );
   }
#endif
   
   // if there's nothing here, can't remove anything

   if( _q_size(que) == 0 ) {
      return( NULL );
   }
      
   // get the data pointer from the first entry

   void *data = _q_peek( que );
   
   // unlink this entry from the queue

   qnode_t *tmp = que->head;
   
   que->head = tmp->next;
   que->size -= 1;
   
   // if this was the last entry, make sure the tail pointer is fixed

   if( que->size == 0 ) {
      que->tail = NULL;
   }
   
   // release the qnode!

   _qnode_free( tmp );
   
   // send the data pointer back to the caller

   return( data );
}

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
                    int (*find)(const void*,const void*) ) {

   qnode_t *prev, *curr;

#if defined(SANITY_CHECK)
   // verify that we got an actual pointer

   if( que == NULL ) {
      _kpanic( "_q_remove_by", "NULL argument" );
   }
#endif

   // traverse the entry list, looking for the victim

   prev = NULL;
   curr = que->head;
   
   while( curr ) {

      // did we find it?

      if( find(data,curr->data) ) {

         // yes - remember the data field

         void *tmp = curr->data;

         // unlink this qnode from the list

         if( prev != NULL ) {
            prev->next = curr->next;
         } else {
            que->head = curr->next;
         }

         // was this the last node?

         if( curr->next == NULL ) {
            que->tail = prev;
         }

         // one fewer occupant

         que->size -= 1;

         // release the qnode

         _qnode_free( curr );

         // return the data

         return( tmp );
      }

      // not found yet - move down the list
      prev = curr;
      curr = curr->next;
   }
   
   // we never found the entry - just return NULL

   return( NULL );
}

/*
** _q_init()
**
** initialize the queue module
*/

void _q_init( void ) {
   // any initialization stuff we think of later
   
   c_puts( " QUEUE" );

   // set up the list of available qnodes

   _free_qnodes = NULL;

   // create the sleeping, zombie, and waiting queues
   
   _sleeping = _q_alloc( NULL );
   if( _sleeping == NULL ) {
      _kpanic( "_q_init", "_sleeping alloc failed" );
   }
   
   _zombie = _q_alloc( NULL );
   if( _zombie == NULL ) {
      _kpanic( "_q_init", "_zombie alloc failed" );
   }
   
   _waiting = _q_alloc( NULL );
   if( _waiting == NULL ) {
      _kpanic( "_q_init", "_waiting alloc failed" );
   }

}

/*
** _q_prune()
**
** release any allocated space we aren't currently using
*/

uint32_t _q_prune( void ) {
   uint32_t sum = 0;

   while( _free_qnodes != NULL ) {
      qnode_t *tmp = _free_qnodes;
      _free_qnodes = tmp->next;
      _kfree( tmp );
      ++sum;
   }

   // return a byte count

   return( sum * sizeof(qnode_t) );
}

/*
** _q_dump(msg,que)
**
** dump the contents of a queue to the console
*/

void _q_dump( const char *msg, queue_t que ) {
   qnode_t *tmp;
   int i;

   c_printf( "%s: ", msg );
   if( que == NULL ) {
      c_puts( "NULL???\n" );
      return;
   }

   c_printf( "head %08x tail %08x %d items",
            (uint32_t) que->head, (uint32_t) que->tail, que->size );

   if( que->compare ) {
      c_printf( " cmp %08x\n", (uint32_t) que->compare );
   } else {
      c_puts( " FIFO\n" );
   }

   if( que->size > 0 ) {
      c_puts( " data: " );
      i = 0;
      for( tmp = que->head; i < 5 && tmp != NULL; ++i, tmp = tmp->next ) {
         c_printf( " [%08x]", (uint32_t) tmp->data );
      }

      if( tmp != NULL ) {
         c_puts( " ..." );
      }

      c_puts( "\n" );
   }
}

/*
** _q_dump_all(msg)
**
** dump the contents of all the queues to the console
*/

void _q_dump_all( const char *msg ) {

   // dump the full contents of all the queues

   if( msg ) {
      c_puts( msg );
   }

   char buf[16];
   _kstrcpy( buf, "ready[N]" );

   // can only handle 10 priorities (0..9)
   int max = N_PRIOS < 11 ? N_PRIOS : 10;
   for( int i = 0; i < max; ++i ) {
      buf[6] = i + '0';
      _q_dump( buf, _ready[i] );
   }

   _q_dump( "sleeping", _sleeping );
   _q_dump( "waiting", _waiting );
   _q_dump( "zombie", _zombie );
   _q_dump( "reading", _reading );

   // add others here as needed

}

/*
** ORDERING FUNCTIONS
*/

/*
** _q_order_wakeup(a1,a2)
**
** used to order a queue by wakeup time
*/

int _q_order_wakeup( const void *a1, const void *a2 ) {

   // parameters will be pointers to PCBS

   const pcb_t *p1 = (const pcb_t *) a1;
   const pcb_t *p2 = (const pcb_t *) a2;
   
   // wakeup times are unsigned 32-bit integers

   if( p1->event < p2->event ) {
      return( -1 );
   } else if( p1->event == p2->event ) {
      return( 0 );
   } else {
      return( 1 );
   }
}

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

int _q_find_pid( const void *data, const void *proc ) {
   const pcb_t *p1 = (const pcb_t *) proc;
   uint32_t ldata = (uint32_t) data & 0x0000ffff;
   
   return( ldata == p1->pid );
}

/*
** _q_find_ppid(data,proc)
**
** find a queue entry with the specified parent PID
**
** unlike PIDs, parent PIDs may not be unique in a queue, so we
** find the first entry having this PPID
*/

int _q_find_ppid( const void *data, const void *proc ) {
   const pcb_t *p1 = (const pcb_t *) proc;
   uint32_t ldata = (uint32_t) data & 0x0000ffff;
   
   return( ldata == p1->ppid );
}
