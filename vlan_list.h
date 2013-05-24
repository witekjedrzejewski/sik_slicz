/* list of vlan ids */

#ifndef _SIK_VLAN_LIST_H_
#define	_SIK_VLAN_LIST_H_

#include <stdint.h>

/* list of tagged vlans */
typedef struct vlan_node vlan_list_t;

/* checks if v exists in vlan */
int vlan_list_exists(vlan_list_t* list, uint16_t v);

/* adds vlan to list, returns err_code */
int vlan_list_add(vlan_list_t** list_ptr, uint16_t v);

/* removes all vlan list from memory */
void delete_vlan_list(vlan_list_t* list);

/* prints vlan list to buffer */
void vlan_list_print(char* buf, vlan_list_t* list);

#endif
