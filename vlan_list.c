#include <stdlib.h>
#include <stdio.h>

#include "error_codes.h"
#include "vlan_list.h"

struct vlan_node {
	uint16_t vlan;
	struct vlan_node* next;
};

int vlan_list_exists(vlan_list_t* list, uint16_t v) {
	vlan_list_t* elt = list;
	while (elt != NULL && elt->vlan != v)
		elt = elt->next;
	return (elt != NULL);
}

int vlan_list_add(vlan_list_t** list_ptr, uint16_t v) {
	if (vlan_list_exists(*list_ptr, v))
		return ERR_VLAN_DUP;

	vlan_list_t* new = malloc(sizeof (vlan_list_t));
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

void delete_vlan_list(vlan_list_t* list) {
	if (list != NULL) {
		delete_vlan_list(list->next); /* recursive */
		free(list);
	}
}

void vlan_list_print(char* buf, vlan_list_t* list) {
	vlan_list_t* elt = list;
	int w = 0;
	while (elt != NULL) {
		w += sprintf(buf + w, "%dt,", elt->vlan);
		elt = elt->next;
	}
}