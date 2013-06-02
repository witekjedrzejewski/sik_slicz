/* Ethernet frame */

#ifndef _SIK_FRAME_H_
#define	_SIK_FRAME_H_

#include "macs_map.h"

#define MAX_CONTENT_SIZE 1502
#define MAX_FRAME_SIZE 1518
#define MIN_FRAME_SIZE 18

/* frame */
typedef struct frame {
	mac_t dst_mac; /* destination mac */
	mac_t src_mac; /* source mac */
	uint16_t tpid; /* Tag Protocol Identifier (empty if untagged) */
	uint16_t tci; /* Tag Control Information (empty if untagged) */
	char content[MAX_CONTENT_SIZE]; /* ether_type + payload */
} __attribute__((packed)) frame_t;

/* checks if frame is tagged 802.1q */
int frame_is_tagged(frame_t* f);

/* returns frame's vlan (if none, undefined) */
int frame_vlan(frame_t* f);

/* returns frame from buffer */
frame_t* frame_from_str(char* buf);

/* fills buffer with frame data */
void frame_to_str(frame_t* f, char* buf);

/* returns source MAC */
mac_t frame_src_mac(frame_t* f);

/* returns destination MAC */
mac_t frame_dst_mac(frame_t* f);

/* removes 802.1q tag (if any) */
void frame_untag(frame_t* f);

/* sets vlan */
void frame_set_vlan(frame_t* f, int vlan);

#endif

