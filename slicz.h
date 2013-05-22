#ifndef _SIK_SLICZ_H_
#define _SIK_SLICZ_H_

#include <event2/event.h>

#define MAX_COMMAND_LINE_LENGTH 250

/* returns event context */
struct event_base* get_base();

#endif