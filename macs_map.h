#ifndef _SIK_MACS_MAP_H_
#define	_SIK_MACS_MAP_H_

#include "ports.h" /* mainly for slicz_port_t declaration */

typedef struct mac_addr_t mac_t;

/* adds or updates record*/
void macs_map_set(mac_t mac, int vlan, slicz_port_t* port);

/* checks if record for given key exists */
int macs_map_exists(mac_t mac, int vlan);

/* returns port for pair (mac, vlan).
 use only after asserting port existance! */
slicz_port_t* macs_map_get(mac_t mac, int vlan);

/* deletes or records concerning given port */
void macs_map_delete_all_by_port(slicz_port_t* port);

#endif

