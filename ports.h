/* list of slicz's ports and everything that concerns them */

#ifndef _SIK_PORTS_H_
#define	_SIK_PORTS_H_

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

typedef struct port_node port_list_t;
typedef struct slicz_port slicz_port_t;

/* sets port configuration, returns err code */
int setconfig(char* config);

/* returns pointer to first port */
port_list_t* port_list_get_first();

/* returns pointer to next port */
port_list_t* port_list_get_next(port_list_t* ptr);

/* fills buf with description of pointed port */
void print_port_description(char* buf, port_list_t* ptr);

/* fills fields with port's counters' values */
void get_port_counters(port_list_t* ptr, uint16_t* port, 
				unsigned* recv, unsigned* sent, unsigned* errs);

#endif

