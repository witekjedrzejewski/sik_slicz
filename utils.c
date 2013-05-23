#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <event2/event.h>

#include "error_codes.h"
#include "utils.h"

int starts_with(char* text, char* prefix) {
	size_t i;
	size_t text_len = strlen(text);
	size_t prefix_len = strlen(prefix);
	for (i = 0; i < prefix_len; i++) {
		if (i >= text_len || text[i] != prefix[i])
			return 0;
	}
	return 1;
}

int sockaddr_from_host_port(char* host, char* port, 
				struct sockaddr_in* result, socklen_t* len) {
	
	struct addrinfo* addr_result = NULL;
	struct addrinfo addr_hints;
  memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET; // IPv4
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;
  if (getaddrinfo(host, port, &addr_hints, &addr_result) != 0)
		return ERR_ADDR_WRONG;
	
	memset(result, 0, sizeof(result));
	memcpy(result, addr_result->ai_addr, addr_result->ai_addrlen);
	*len = addr_result->ai_addrlen;
	
  freeaddrinfo(addr_result);
	return OK;
}