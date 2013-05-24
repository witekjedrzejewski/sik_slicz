/* 
 * File:   test_frame_queue.c
 * Author: wit
 *
 * Created on May 24, 2013, 1:38 PM
 */

#include <stdio.h>
#include <stdlib.h>

#include "frame_queue.h"

int main(int argc, char** argv) {

	frame_queue_t* q1 = frame_queue_new();
	
	frame_queue_push(q1, 3);
	printf("%d\n", frame_queue_pop(q1));
	
	int i;
	for (i = 1; i < 10; i++)
		frame_queue_push(q1, i*10);
	
	while (!frame_queue_is_empty(q1))
		printf("pop %d\n", frame_queue_pop(q1));
	
	
	
	return (EXIT_SUCCESS);
}

