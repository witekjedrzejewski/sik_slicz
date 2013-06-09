#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <linux/if_ether.h>
#include <netinet/in.h>

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

frame_t* frame_from_str(char* buf, size_t len) {
	frame_t* f = malloc(sizeof (frame_t));
	memcpy(f, buf, len);
	
	f->content_len = len - TAG_END;
	
	if (!frame_is_tagged(f)) {
		f->content_len = len - MACS_END;
		/* move content to content section */
		memmove(f + TAG_END, f + MACS_END, f->content_len);
	}
	return f;
}

size_t frame_to_str(frame_t* f, char* buf) {
	buf = (char*) f;
	size_t len = TAG_END;
	if (!frame_is_tagged(f)) {
		/* pull content back to header */
		memmove(buf + MACS_END, buf + TAG_END, f->content_len);
		len = MACS_END;
	}
	len += f->content_len;
	return len;
}

void frame_set_vlan(frame_t* f, int vlan) {
	if (!frame_is_tagged(f)) {
		f->tpid = htons(ETH_P_8021Q);
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

void print_frame_header(frame_t* f) {
	char smac[18], dmac[18];
	mac_to_str(f->src_mac, smac);
	mac_to_str(f->dst_mac, dmac);
	printf("%s | %s | %d\n", smac, dmac, 
					frame_is_tagged(f) ? frame_vlan(f) : 0);
}