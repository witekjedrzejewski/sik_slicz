#include "ports.h"

#define MIN_VLAN 1
#define MAX_VLAN 4095

int config(char* lis_port, char* recv_addr, char* recv_port, vlan_list_t)

/* parses data to vlan list + optionally untagged vlan, returns 0 on success */
int vlan_list_from_string(char* data, vlan_list_t* list, int* untagged) {
	*untagged = -1;
	char* v = strtok(vlan_data, ",");
	while (v != NULL) {
		if (v[strlen(v)-1] == 't') {/* tagged */
			int vlan = atoi(vlan); /* t is trimmed */
			if (vlan < MIN_VLAN || vlan > MAX_VLAN)
				return 1;
			if (vlan_list_add(vlan) != 0)
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

	/* important to seperate strtok calls */
	if (is_recv_given) {
		recv_addr = strtok(recv_data, ":");
		if (recv_addr == NULL)
			return 1;
		recv_port = strtok(NULL, ":");
		if (recv_port == NULL)
			return 1;
	}
	
	vlan_list_t vlan_list = NULL;
	int untagged;
	if (vlan_list_from_string(vlan_data, &vlan_list, &untagged) != 0)
		return 1;

	return config_port(lis_port, recv_addr, recv_port, vlan_list, untagged);
}
