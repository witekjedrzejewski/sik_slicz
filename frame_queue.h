#ifndef _SIK_FRAME_QUEUE_H_
#define	_SIK_FRAME_QUEUE_H_

typedef struct frame_queue frame_queue_t;
typedef int frame_t;

/* returns new empty queue */
frame_queue_t* frame_queue_new();

/* checks if queue is empty */
int frame_queue_is_empty(frame_queue_t* q);

/* pushes frame f at the end of queue q */
void frame_queue_push(frame_queue_t* q, frame_t f);

/* removes and returns first frame from queue.
 * Use ONLY if queue not empty! */
frame_t frame_queue_pop(frame_queue_t* q);

#endif
