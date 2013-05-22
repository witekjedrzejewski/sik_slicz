#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <event2/event.h>
#include <arpa/inet.h>

#include "ports.h"
#include "slicz.h"
#include "error_codes.h"

#define MIN_VLAN 1
#define MAX_VLAN 4095
#define MAX_ACTIVE_PORTS 100
#define MAX_ADDRESS_LENGTH 100


/* TODO kolejnosc, duplikaty w obrebie portow */

/* list of all ports */
port_list_t* port_list = NULL;

/* number of active listening ports */
int active_ports = 0;

static void delete_vlan_list(vlan_list_t* list) {
	//todo
}

/* adds port to port_list, or replaces already existing, returns err code */
static int port_list_add(uint16_t lis_port, int is_bound, 
				struct sockaddr_in receiver, vlan_list_t* vlans, int untagged) {
	
	printf("port_list_add(port = %d)\n", lis_port);
	
	port_list_t* elt = port_list;
	while (elt != NULL && elt->port->lis_port != lis_port)
		elt = elt->next;
	
	/* TODO powtorzenie portu */
	
	slicz_port_t* new_port;
	
	if (elt == NULL) { /* not found, we creating new */
		if (active_ports >= MAX_ACTIVE_PORTS)
			return ERR_TOO_MANY;
		struct sockaddr_in lis_addr;
		lis_addr.sin_family = AF_INET;
		lis_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		lis_addr.sin_port = htons(lis_port);
		int udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
		if (udp_sock < 0)
			return ERR_SOCK;
		if (bind(udp_sock, (struct sockaddr*)&lis_addr, 
						(socklen_t) sizeof(lis_addr)) < 0)
			return ERR_SOCK;

		new_port = malloc(sizeof(slicz_port_t));
		new_port->sock = udp_sock;
/*		new_port->read_event = event_new(get_base(), udp_sock, EV_READ|EV_PERSIST,
      read_frame_event, (void*) new_port);
		new_port->write_event = event_new(get_base(), udp_sock, EV_WRITE|EV_PERSIST,
      write_frame_event, (void*) new_port);
	*/	
		
		elt = malloc(sizeof(port_list_t));
		elt->next = NULL;

	} else {
		delete_vlan_list(elt->port->vlan_list);
		new_port = elt->port;
	}
	
	new_port->lis_port = lis_port;
	new_port->vlan_list = vlans;
	new_port->untagged = untagged;
	new_port->is_bound = is_bound;
	new_port->receiver = receiver;
	new_port->recv = new_port->sent = new_port->errs = 0;
	
	elt->port = new_port;
	
	return OK;
}

/* checks if v exists in vlan */
static int vlan_list_exists(vlan_list_t* list, uint16_t v) {
	vlan_list_t* elt = list;
	while (elt != NULL && elt->vlan != v)
		elt = elt->next;
	return (elt != NULL);
}

/* adds vlan to list, returns 0 on success, 'throws' when value repeats */
static int vlan_list_add(vlan_list_t** list_ptr, uint16_t v) {
	printf("vlan_list_add(%d)\n", v);
	if (vlan_list_exists(*list_ptr, v))
		return ERR_VLAN_DUP;
	
	printf("not exitsts\n");
	
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
	printf("vlan_list ok\n");
	return OK;
}

/* fills sockaddr basing on host and port, returns err_code */
static int sockaddr_from_host_port(char* host, char* port, 
				struct sockaddr_in* result) {
	
	struct addrinfo* addr_result = NULL;
	struct addrinfo addr_hints;
  memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET; // IPv4
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;
  if (getaddrinfo(host, port, &addr_hints, &addr_result) != 0)
		return ERR_ADDR_WRONG;
	
	result->sin_family = AF_INET; // IPv4
  result->sin_addr.s_addr =
      ((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr;
  result->sin_port = htons((uint16_t) atoi(port));

  freeaddrinfo(addr_result);
	return OK;
}

int setconfig(char* config) {
	char* lis_port = NULL;
	char* recv_data = NULL;
	char* recv_addr = NULL;
	char* recv_port = NULL;
	char* vlan_data = NULL;
	
	printf("setconfig [%s]\n", config);
	/* is receiver data given */
	int is_recv_given = (strstr(config, "//") == NULL) ? 1 : 0;
	
	lis_port = strtok(config, "/");
	if (lis_port == NULL)
		return ERR_PORT_NONE;
	
	printf("lis_port = %s\n", lis_port);
	
	if (is_recv_given) {
		recv_data = strtok(NULL, "/");
		if (recv_data == NULL)
			return ERR_RECV_NONE;
		printf("recv_data = %s\n", recv_data);
	}
	
	vlan_data = strtok(NULL, "/");
	if (vlan_data == NULL)
		return ERR_VLAN_NONE;

	printf("vlan_data = %s\n", vlan_data);
	
	struct sockaddr_in receiver;
	/* important to seperate strtok calls */
	if (is_recv_given) {
		recv_addr = strtok(recv_data, ":");
		if (recv_addr == NULL)
			return ERR_RECV_NONE;
		recv_port = strtok(NULL, ":");
		if (recv_port == NULL)
			return ERR_RECV_NONE;
		
		int err = sockaddr_from_host_port(recv_addr, recv_port, &receiver);
		if (err != OK)
			return err;
	}
	
	vlan_list_t* vlan_list = NULL;
	int untagged;
	int err = vlan_list_from_string(vlan_data, &vlan_list, &untagged);
	if (err != 0)
		return err;

	return port_list_add(atoi(lis_port), is_recv_given, 
					receiver, vlan_list, untagged);
}

port_list_t* port_list_get_first() {
	return port_list;
}

port_list_t* port_list_get_next(port_list_t* ptr) {
	return ptr->next;
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
}

/* returns description of pointed port */
void port_list_print_port(char* buf, port_list_t* ptr) {
	char vlan_buf[MAX_COMMAND_LINE_LENGTH];
	char addr_buf[MAX_ADDRESS_LENGTH];
	int w = sprintf(buf, "%d/", ptr->port->lis_port);
  if (ptr->port->is_bound) {
		inet_ntop(AF_INET, &(ptr->port->receiver), 
						addr_buf, INET_ADDRSTRLEN);
		w += sprintf(buf + w, "%s:", addr_buf);
		int recv_port = ntohs(((struct sockaddr_in)ptr->port->receiver).sin_port);
		w += sprintf(buf + w, "%d", recv_port);
	}
	vlan_list_print(vlan_buf, ptr->port);
	sprintf(buf + w, "%s", vlan_buf);
}
