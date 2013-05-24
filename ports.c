#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <event2/event.h>
#include <arpa/inet.h>

#include "utils.h"
#include "ports.h"
#include "slicz.h"
#include "error_codes.h"
#include "err.h"
#include "macs_map.h"

#define MIN_VLAN 1
#define MAX_VLAN 4095
#define MAX_ACTIVE_PORTS 100
#define MAX_ADDRESS_LENGTH 100
#define MAX_FRAME_LENGTH 1518 /* including header, basing on example */

/* list of tagged vlans */
typedef struct vlan_node {
	uint16_t vlan;
	struct vlan_node* next;
} vlan_list_t;

/* single listening port in switch */
struct slicz_port {
	uint16_t lis_port; /* listening port, host-bit-order */
	vlan_list_t* vlan_list; /* list of tagged vlans */
	int untagged; /* untagged vlan (-1 if none) */
	int is_bound; /* is receiver data set */
	
	/* receiver data */
	struct sockaddr_in receiver;
	socklen_t recv_len;
	
	int sock; /* sock descriptor */
	
	/* events */
	struct event* read_event;
	struct event* write_event;
	/* counters */
	unsigned recv;
	unsigned sent;
	unsigned errs;
	
};

/* list of listening port */
struct port_node {
	slicz_port_t* port;
	struct port_node* next;
};

/* list of all ports */
port_list_t* port_list = NULL;

/* number of active listening ports */
int active_ports = 0;

static void delete_vlan_list(vlan_list_t* list) {
	if (list != NULL) {
		delete_vlan_list(list->next); /* recursive */
		free(list);
	}
}

static void vlan_list_print(char* buf, slicz_port_t* port) {
	int w = 0;
	if (port->untagged != -1)
		w = sprintf(buf, "%d,", port->untagged);

	vlan_list_t* elt = port->vlan_list;
	while (elt != NULL) {
		w += sprintf(buf + w, "%dt,", elt->vlan);
		elt = elt->next;
	}
	
	/* delete last comma */
	buf[w-1] = '\0';
}

static void bind_port_with_addr(slicz_port_t* port) {
	if (connect(port->sock, (struct sockaddr *)&(port->receiver), 
					port->recv_len) < 0)
		syserr("Connecting udp socket");
	port->is_bound = 1;
}

static void unbind_port_and_addr(slicz_port_t* port) {
	if (!port->is_bound)
		return;
	port->is_bound = 0;
	memset(&port->receiver, 0, port->recv_len);
	port->receiver.sin_family = AF_UNSPEC;
	if (connect(port->sock, (struct sockaddr *)&(port->receiver), 
					port->recv_len) < 0)
		syserr("Disconnecting udp socket");
}


static void read_frame_event(evutil_socket_t sock, short ev, void* arg) {
  
	slicz_port_t* port = (slicz_port_t*) arg;
  char buf[MAX_FRAME_LENGTH];
	int r;
	if (port->is_bound) {
		r = recv(sock, buf, sizeof(buf), MSG_DONTWAIT);
		if (r < 0) {/* reading failed - we remove bound address */
			unbind_port_and_addr(port);
		}
	} else {
		struct sockaddr_in sender;
		socklen_t sender_len = (socklen_t) sizeof(sender);
		r = recvfrom(sock, buf, sizeof(buf), MSG_DONTWAIT,
        (struct sockaddr*) &sender, &sender_len);
		if (r < 0)
			return;
		memcpy(&port->receiver, &sender, sender_len);
		port->recv_len = sender_len;
		bind_port_with_addr(port);
	}
	
	printf("%d -> [%s]\n", port->lis_port, buf);
	
	/* TODO cos z ramkami */
}

/* if is_new, prepares new port, in other case updates port configuration */
static void prepare_port(slicz_port_t** port, int is_new, uint16_t lis_port,
				int is_bound, struct sockaddr_in receiver, socklen_t recv_len,
				vlan_list_t* vlans, int untagged) {
	(*port)->lis_port = lis_port;
	(*port)->vlan_list = vlans;
	(*port)->untagged = untagged;
	(*port)->recv = (*port)->sent = (*port)->errs = 0;
	
	if (is_new) {
		printf("creating new elt\n");
		
		int udp_sock;
		int err = setup_udp(&udp_sock, lis_port);
		if (err != OK)
			syserr("Setup udp socket");
	
		(*port)->sock = udp_sock;
		
		(*port)->read_event = event_new(get_base(), udp_sock, EV_READ|EV_PERSIST,
      read_frame_event, (void*) *port);
		/*(*port)->write_event = event_new(get_base(), udp_sock, EV_WRITE|EV_PERSIST,
      write_frame_event, (void*) *port);
		*/
	} else { 
		delete_vlan_list((*port)->vlan_list);
		macs_map_delete_all_by_port(*port); /* delete all concerning mac records */
		unbind_port_and_addr(*port);
	}
	
	(*port)->is_bound = 0; /* new or just unbound */
	
	/* important to call AFTER unbind */
	(*port)->receiver = receiver; 
	(*port)->recv_len = recv_len;
	if (is_bound) /* not port->is_bound */
		bind_port_with_addr(*port);
	
	event_add((*port)->read_event, NULL); /* no timeout */
}

/* checks if listening port is on list */
static int port_list_exists(uint16_t lis_port) {
	port_list_t* elt = port_list;
	while (elt != NULL && elt->port->lis_port != lis_port)
		elt = elt->next;
	return elt != NULL;
}

/* adds port to port_list, or replaces already existing, 
 * returns 1 if added, 0 if replaced */
static int port_list_add(uint16_t lis_port, slicz_port_t** port) {
	
	port_list_t* elt = port_list;
	port_list_t* last = NULL;
	
	while (elt != NULL && elt->port->lis_port < lis_port) {
		last = elt;
		elt = elt->next;
	}
	
	if (elt == NULL || elt->port->lis_port > lis_port) {/* inserting */
		port_list_t* new_node = malloc(sizeof(port_list_t));
		*port = malloc(sizeof(slicz_port_t));
		new_node->port = *port;
		if (last == NULL) {/* adding from front */
			new_node->next = elt;
			port_list = new_node;
		} else {
			new_node->next = last->next;
			last->next = new_node;
		}
		return 1;
	} else { /* lis_ports equal, replacing */
		*port = elt->port;
		return 0;
	}
}

/* checks if v exists in vlan */
static int vlan_list_exists(vlan_list_t* list, uint16_t v) {
	vlan_list_t* elt = list;
	while (elt != NULL && elt->vlan != v)
		elt = elt->next;
	return (elt != NULL);
}

/* adds vlan to list, returns err_code */
static int vlan_list_add(vlan_list_t** list_ptr, uint16_t v) {
	if (vlan_list_exists(*list_ptr, v))
		return ERR_VLAN_DUP;
	
	vlan_list_t* new = malloc(sizeof(vlan_list_t));
	new->vlan = v;
	new->next = NULL;
	
	if (*list_ptr == NULL) {
		*list_ptr = new;
	} else {
		new->next = *list_ptr;
		*list_ptr = new;
	}
		
	return OK;
}

/* parses data to vlan list or untagged vlan, returns 0 on success */
static int vlan_list_from_string(char* data, vlan_list_t** list_ptr, 
				int* untagged) {
	*untagged = -1;
	char* v = strtok(data, ",");
	while (v != NULL) {
		uint16_t vlan = atoi(v); /* t is trimmed */
		if (vlan < MIN_VLAN || vlan > MAX_VLAN)
			return ERR_VLAN_BOUNDS;
		
		if (v[strlen(v)-1] == 't') {/* tagged */
			int err = vlan_list_add(list_ptr, vlan);
			if (err != 0)
				return err;
		} else {
			if (*untagged == -1)
				*untagged = vlan;
			else /* more than one untagged vlan */
				return ERR_VLAN_MANY_U;
		}
		v = strtok(NULL, ",");
	}
	if (*list_ptr == NULL && *untagged == -1) /* no vlans */
		return ERR_VLAN_NONE;
	if (vlan_list_exists(*list_ptr, *untagged)) 
		return ERR_VLAN_DUP;
	return OK;
}

int setconfig(char* config) {
	char* lis_port = NULL;
	char* recv_data = NULL;
	char* recv_addr = NULL;
	char* recv_port = NULL;
	char* vlan_data = NULL;
	
	int err;
	
	/* is receiver data given */
	int is_recv_given = (strstr(config, "//") == NULL) ? 1 : 0;
	
	lis_port = strtok(config, "/");
	if (lis_port == NULL)
		return ERR_PORT_NONE;
	
	
	if (is_recv_given) {
		recv_data = strtok(NULL, "/");
		if (recv_data == NULL)
			return ERR_RECV_NONE;
	}
	
	vlan_data = strtok(NULL, "/");
	if (vlan_data == NULL)
		return ERR_VLAN_NONE;
	
	struct sockaddr_in receiver;
	socklen_t recv_len;
	
	/* important to seperate strtok calls */
	if (is_recv_given) {
		recv_addr = strtok(recv_data, ":");
		if (recv_addr == NULL)
			return ERR_RECV_NONE;
		recv_port = strtok(NULL, ":");
		if (recv_port == NULL)
			return ERR_RECV_NONE;
		
		int err = sockaddr_from_host_port(recv_addr, recv_port, 
						&receiver, &recv_len);
		if (err != OK)
			return err;
	}
	
	vlan_list_t* vlan_list = NULL;
	int untagged;
	err = vlan_list_from_string(vlan_data, &vlan_list, &untagged);
	if (err != OK)
		return err;

	uint16_t lis_port_num = atoi(lis_port);
	
	int is_new = !(port_list_exists(lis_port_num));
	if (is_new && active_ports >= MAX_ACTIVE_PORTS)
		return ERR_TOO_MANY; /* we do it here, before we allocate smth */
	
	slicz_port_t* port_node;
	port_list_add(lis_port_num, &port_node);
	prepare_port(&port_node, is_new, lis_port_num, is_recv_given, 
					receiver, recv_len, vlan_list, untagged);
	
	active_ports++;
	return OK;
}

port_list_t* port_list_get_first() {
	return port_list;
}

port_list_t* port_list_get_next(port_list_t* ptr) {
	return ptr->next;
}

void print_port_description(char* buf, port_list_t* ptr) {
	char vlan_buf[MAX_COMMAND_LINE_LENGTH];
	char addr_buf[MAX_ADDRESS_LENGTH];
	int w = sprintf(buf, "%d/", ptr->port->lis_port);
  if (ptr->port->is_bound) {
		inet_ntop(AF_INET, &(ptr->port->receiver.sin_addr), 
						addr_buf, INET_ADDRSTRLEN);
		w += sprintf(buf + w, "%s:", addr_buf);
		int recv_port = ntohs(ptr->port->receiver.sin_port);
		w += sprintf(buf + w, "%d", recv_port);
	}
	vlan_list_print(vlan_buf, ptr->port);
	sprintf(buf + w, "/%s", vlan_buf);
}

void get_port_counters(port_list_t* ptr, uint16_t* port, 
				unsigned* recv, unsigned* sent, unsigned* errs) {
	
	*port = ptr->port->lis_port;
	*recv = ptr->port->recv;
	*sent = ptr->port->sent;
	*errs = ptr->port->errs;
}
