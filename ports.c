#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ports.h"

#define MIN_VLAN 1
#define MAX_VLAN 4095

/* list of all ports */
port_list_t* port_list = NULL;

void delete_vlan_list(vlan_list_t* list) {
	//todo
}

/* adds port to port_list, or replaces already existing */
void port_list_add(uint16_t lis_port, int is_bound, struct sockaddr receiver,
				vlan_list_t* vlans, uint16_t untagged) {
	
	vlan_list_t* elt = port_list;
	while (elt != NULL && elt->port->lis_port != lis_port)
		elt = elt->next;
	
	slicz_port_t* new_port;
	
	if (elt == NULL) { /* not found, we creating new */
		struct sockaddr_in lis_addr;
		lis_addr.sin_family = AF_INET;
		lis_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		lis_addr.sin_port = htons(lis_port);
		int udp_sock = socket(PF_INET, SOCK_DGRAM, 0);
		if (udp_sock < 0) {
			return 1;
		if (bind(sock, (struct sockaddr*)&lis_addr, 
						(socklen_t) sizeof(lis_addr) < 0) {
			return 1;

		new_port = malloc(sizeof(slicz_port_t));
		new_port->sock = udp_sock;
		new_port->read_event = event_new(get_base(), udp_sock, EV_READ|EV_PERSIST,
      read_frame_event, (void*) new_port);
		new_port->write_event = event_new(get_base(), udp_sock, EV_WRITE|EV_PERSIST,
      write_frame_event, (void*) new_port);
		
		
		elt = malloc(sizeof(port_list_t));
		elt->next = NULL;

	} else {
		delete_vlan_list(elt->port->vlan_list);
		new_port = elt->port;
	}
	
	new_port->lis_port = lis_port;
	new_port->vlan_list = vlans;
	new_port->is_bound = is_bound;
	new_port->receiver = receiver;
	new_port->recv = new_port->sent = new_port->errs = 0;
	
	elt->port = new_port;
}

/* checks if v exists in vlan */
int vlan_list_exists(vlan_list_t* list, uint16_t v) {
	vlan_list_t* elt = list;
	while (elt != NULL && elt->vlan != v)
		elt = elt->next;
	return (elt != NULL);
}

/* adds vlan to list, returns 0 on success, 'throws' when value repeats */
int vlan_list_add(vlan_list_t** list_ptr, uint16_t v) {
	if (vlan_list_exists(*list_ptr, v))
		return 1;
	
	vlan_list_t* new = malloc(sizeof(vlan_list_t));
	new->vlan = v;
	new->next = NULL;
	
	if (list == NULL) {
		*list = new;
	} else {
		new->next = *list;
		*list = new;
	}
		
	return 0;
}

/* parses data to vlan list or untagged vlan, returns 0 on success */
int vlan_list_from_string(char* data, vlan_list_t* list, uint16_t* untagged) {
	*untagged = -1;
	char* v = strtok(vlan_data, ",");
	while (v != NULL) {
		if (v[strlen(v)-1] == 't') {/* tagged */
			int vlan = atoi(vlan); /* t is trimmed */
			if (vlan < MIN_VLAN || vlan > MAX_VLAN)
				return 1;
			if (vlan_list_add(list, vlan) != 0)
				return 1; /* error while adding */
		} else {
			if (*untagged == -1)
				*untagged = vlan;
			else /* more than one untagged vlan */
				return 1;
		}
	}
	if (list == NULL && *untagged == -1) /* no vlans */
		return 1;
	if (list != NULL && *untagged != -1) /* both tagged and untagged */
		return 1;
	return 0;
}

/* fills sockaddr basing on host and port, returns 0 on success */
int sockaddr_from_host_port(char* host, char* port, 
				struct sockaddr* result) {
	
	struct addrinfo* addr_result = NULL;
	struct addrinfo addr_hints;
  memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET; // IPv4
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;
  if (getaddrinfo(host, port, &addr_hints, &addr_result) != 0)
		return 1;
	*result = *(addr_result->ai_addr);
	return 0;
}

int setconfig(char* config) {
	char* lis_port = NULL;
	char* recv_data = NULL;
	char* recv_addr = NULL;
	char* recv_port = NULL;
	char* vlan_data = NULL;
	
	/* is receiver data given */
	int is_recv_given = (strstr(config, "//") == NULL) ? 1 : 0;
	
	lis_port = strtok(config, "/");
	if (lis_port == NULL)
		return 1;
	
	if (recv_is_given) {
		recv_data = strtok(NULL, "/");
		if (recv_data == NULL)
			return 1;
	}
	
	vlan_data = strtok(NULL, "/");
	if (vlan_data == NULL)
		return 1;

	struct sockaddr receiver;
	/* important to seperate strtok calls */
	if (is_recv_given) {
		recv_addr = strtok(recv_data, ":");
		if (recv_addr == NULL)
			return 1;
		recv_port = strtok(NULL, ":");
		if (recv_port == NULL)
			return 1;
		
		if (sockaddr_from_host_port(recv_addr, recv_port, &receiver) != 0)
			return 1;
	}
	
	vlan_list_t* vlan_list = NULL;
	uint16_t untagged;
	if (vlan_list_from_string(vlan_data, &vlan_list, &untagged) != 0)
		return 1;

	port_list_add(atoi(lis_port), is_recv_given, receiver, vlan_list, untagged);
	
	return 0;
}
