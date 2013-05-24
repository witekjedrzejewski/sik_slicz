#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "macs_map.h"

#define MACS_MAP_SIZE 4096

typedef struct map_record {
	mac_t mac;
	int vlan;
	slicz_port_t* port;
	time_t insert_time;
	int removed; /* is record removed */
} map_record_t;

map_record_t map[MACS_MAP_SIZE];
size_t map_count = 0; /* number of (unremoved) records */

/* returns if two macs are equal */
static int macs_equal(mac_t m1, mac_t m2) {
	return memcmp(&m1, &m2, sizeof(mac_t)) == 0;
}

/* returns index of free or least recently modified record */
static size_t macs_map_free_index() {
	size_t min_index = 0;
	size_t i;
	for (i = 0; i < MACS_MAP_SIZE; i++) {
		if (map[i].removed)
			return i;
		if (map[i].insert_time < map[min_index].insert_time) {
			min_index = i;
		}
	}
	return min_index;
}

/* returns index of record or -1 on not found */
static size_t macs_map_get_index(mac_t mac, int vlan) {
	size_t i;
	for (i = 0; i < MACS_MAP_SIZE; i++)
		if (!map[i].removed && macs_equal(mac, map[i].mac) && vlan == map[i].vlan)
			return i;
	return -1;
}

void macs_map_set(mac_t mac, int vlan, slicz_port_t* port) {
	size_t index = macs_map_get_index(mac, vlan);
	if (index == -1) {/* no record yet */
		index = macs_map_free_index();
	}
	map[index].removed = 0;
	map[index].mac = mac;
	map[index].vlan = vlan;
	map[index].port = port;
	map[index].insert_time = time(0);
}

int macs_map_exists(mac_t mac, int vlan) {
	return macs_map_get_index(mac, vlan) != -1;
}

slicz_port_t* macs_map_get(mac_t mac, int vlan) {
	return map[macs_map_get_index(mac, vlan)].port;
}

void macs_map_delete_all_by_port(slicz_port_t* port) {
	size_t i;
	for (i = 0; i < MACS_MAP_SIZE; i++) {
		if (map[i].port == port)
			map[i].removed = 1;
	}
}

int mac_is_multicast(mac_t mac) {
	return mac.a[0] & 1;
}