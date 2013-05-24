#ifndef _SIK_FRAME_H_
#define	_SIK_FRAME_H_

#include "macs_map.h"

#define MAX_CONTENT_SIZE 1502
#define MAX_FRAME_SIZE 1518

/* Ethernet frame */
typedef struct frame {
	mac_t dst_mac; /* destination mac */
	mac_t src_mac; /* source mac */
	uint16_t tpid; /* Tag Protocol Identifier (empty if untagged) */
	uint16_t tci;  /* Tag Control Information (empty if untagged) */
	char content[MAX_CONTENT_SIZE]; /* ether_type + payload */
}__attribute__((packed)) frame_t;

int frame_is_tagged(frame_t* f);
int frame_vlan(frame_t* f);
frame_t* frame_from_str(char* buf);
void frame_to_str(frame_t* f, char* buf);
mac_t frame_src_mac(frame_t* f);
mac_t frame_dst_mac(frame_t* f);
void frame_untag(frame_t* f);
void frame_set_vlan(frame_t* f, int vlan);
#endif

