#include <stdlib.h>

#include "frame_queue.h"

#define MAX_QUEUE_SIZE 64000

typedef struct frame_node {
	struct frame_node* next;
	frame_t frame;
} frame_node_t;

struct frame_queue {
	frame_node_t* head;
	frame_node_t* last;
	size_t size;
};

frame_queue_t* frame_queue_new() {
	frame_queue_t* q = malloc(sizeof(frame_queue_t));
	q->head = q->last = NULL;
	q->size = 0;
	return q;
}

int frame_queue_is_empty(frame_queue_t* q) {
	return q->head == NULL;
}

int frame_queue_push(frame_queue_t* q, frame_t* f) {
	if (q->size + sizeof(frame_t) > MAX_QUEUE_SIZE) /* full buffer */
		return 0;
	
	frame_node_t* new_node = malloc(sizeof(frame_node_t));
	new_node->frame = *f;
	new_node->next = NULL;
	if (q->head == NULL) { /* queue empty */
		q->head = new_node;
		q->last = q->head;
	} else {
		q->last->next = new_node;
		q->last = new_node;
	}
	q->size += sizeof(frame_t);
	
	return 1;
}

void frame_queue_pop(frame_queue_t* q, frame_t* frame) {
	frame_t f = q->head->frame;
	frame_node_t* old_head = q->head;
	q->head = q->head->next;
	free(old_head);
	if (q->last == NULL)
		q->last = q->head;
	q->size -= sizeof(frame_t);
	*frame = f;
}