#include <stdint.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <string.h>

#include "frame.h"

#define MACS_END 12 /* mac addresses end after 12 octets from beginning*/
#define TAG_END 16  /* 802.1Q header ends after 16 octets from beginning */
#define VLAN_FILTER 4095U /* first 12 bits checked */

int frame_is_tagged(frame_t* f) {
	return f->tpid == ntohs(ETH_P_8021Q);
}

int frame_vlan(frame_t* f) {
	return ntohs(f->tci) & VLAN_FILTER;
}

frame_t* frame_from_str(char* buf) {
	frame_t* f = (frame_t*) buf;
	if (!frame_is_tagged(f)) {
		size_t content_size = strlen(buf) - MACS_END;
		memmove(f + TAG_END, f + MACS_END, content_size);
	}
	return f;
}

void frame_to_str(frame_t* f, char* buf) {
	buf = (char*) f;
	if (!frame_is_tagged(f)) {
		memmove(buf + MACS_END, buf + TAG_END, strlen(f->content));
	}
}

void frame_set_vlan(frame_t* f, int vlan) {
	if (!frame_is_tagged(f)) {
		f->tpid = ntohs(ETH_P_8021Q);
	}
	f->tci = htons(vlan & VLAN_FILTER);
}

void frame_untag(frame_t* f) {
	if (frame_is_tagged(f))
		f->tpid = 0;
}

mac_t frame_src_mac(frame_t* f) {
	return f->src_mac;
}

mac_t frame_dst_mac(frame_t* f) {
	return f->dst_mac;
}