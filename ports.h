#ifndef _SIK_PORTS_H_
#define	_SIK_PORTS_H_

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>

/* list of tagged vlans */
typedef struct vlan_node {
	uint16_t vlan;
	struct vlan_node* next;
} vlan_list_t;

/* single listening port in switch */
typedef struct slicz_port {
	uint16_t lis_port; /* listening port, host-bit-order */
	vlan_list_t* vlan_list; /* list of tagged vlans */
	int untagged; /* untagged vlan (-1 if none) */
	int is_bound; /* is receiver data set */
	
	/* receiver data */
	struct sockaddr_in receiver;
	
	int sock; /* sock descriptor */
	
	/* events */
	struct event* read_event;
	struct event* write_event;
	/* counters */
	unsigned recv;
	unsigned sent;
	unsigned errs;
	
} slicz_port_t;

/* list of listening port */
typedef struct port_node {
	slicz_port_t* port;
	struct port_node* next;
} port_list_t;

/* sets port configuration, returns err code */
int setconfig(char* config);

/* returns pointer to first port */
port_list_t* port_list_get_first();

/* returns pointer to next port */
port_list_t* port_list_get_next(port_list_t* ptr);

/* fills bud with description of pointed port */
void port_list_print_port(char* buf, port_list_t* ptr);

#endif

