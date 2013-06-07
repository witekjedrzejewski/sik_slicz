/* FIFO queue of frames */

#ifndef _SIK_FRAME_QUEUE_H_
#define	_SIK_FRAME_QUEUE_H_

#include "frame.h"

/* queue */
typedef struct frame_queue frame_queue_t;

/* returns new empty queue */
frame_queue_t* frame_queue_new();

/* checks if queue is empty */
int frame_queue_is_empty(frame_queue_t* q);

/* tries to push frame f at the end of queue q,
 * return false if buffer is full */
int frame_queue_push(frame_queue_t* q, frame_t* f);

/* removes and returns first frame from queue.
 * Use ONLY if queue not empty! */
void frame_queue_pop(frame_queue_t* q, frame_t* f);

/* deletes entire queue from memory */
void frame_queue_delete_all(frame_queue_t* q);

#endif
