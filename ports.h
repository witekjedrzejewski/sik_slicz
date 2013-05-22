#ifndef _SIK_PORTS_H_
#define	_SIK_PORTS_H_

/* list of tagged vlans */
typedef struct vlan_node {
	int vlan;
	struct vlan_node* next;
} vlan_list_t;

/* single listening port in switch */
typedef struct slicz_port {
	uint16_t lis_port; /* listening port, host-bit-order */
	vlan_list_t* vlan_list; /* list of tagged vlans */
	uint16_t untagged; /* untagged vlan (-1 if none) */
	int is_bound; /* is receiver data set */
	struct sockaddr receiver; /* receiver data */
	
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

/* sets port configuration, returns 0 on success */
int setconfig(char* config);

#endif

