#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <errno.h>

#include <event2/event.h>
#include <event2/util.h>
#include <getopt.h>

#include "err.h"

#define MAX_FRAME_SIZE 1518
#define MAX_TAP_NAME_LENGTH 50
#define DEFAULT_TAP_NAME "siktap"

int tap_fd;
int slicz_fd;
char buf[MAX_FRAME_SIZE];
char tap_name[MAX_TAP_NAME_LENGTH];

char* slicz_addr = NULL;
char* slicz_port = NULL;

void slicz_read_callback(evutil_socket_t sock, short ev, void* arg) {
	printf("odbieram\n");
	int r = recv(slicz_fd, buf, sizeof (buf), 0);
	if (r < 0) syserr("Slicz socket read");

	int w = write(tap_fd, buf, r);
	if (w < 0) syserr("Tap socket write");
}

void tap_read_callback(evutil_socket_t sock, short ev, void* arg) {
	printf("wysylam\n");
	int r = read(tap_fd, buf, sizeof (buf));
	if (r < 0) syserr("Tap socket read");

	int w = send(slicz_fd, buf, r, 0);
	if (w < 0) syserr("Slicz socket write");
}

int main(int argc, char** argv) {

	char c;

	strcpy(tap_name, DEFAULT_TAP_NAME);
	while ((c = getopt(argc, argv, "d:")) != -1) {
		switch (c) {
			case 'd':
				if (strlen(optarg) >= MAX_TAP_NAME_LENGTH)
					fatal("Too long interface name");
				strcpy(tap_name, optarg);
				break;
			default:
				fatal("Unknown option");
		}
	}

	if (optind >= argc) fatal("Wrong configuration");

	slicz_addr = strtok(argv[optind], ":");
	if (slicz_addr == NULL) fatal("No address");
	slicz_port = strtok(NULL, ":");
	if (slicz_port == NULL) fatal("No port");

	/* INIT TAP */

	struct ifreq ifr;
	int err;

	if ((tap_fd = open("/dev/net/tun", O_RDWR)) < 0) {
		perror("open(/dev/net/tun)");
		return 1;
	}

	memset(&ifr, 0, sizeof (ifr));
	strncpy(ifr.ifr_name, tap_name, IFNAMSIZ);

	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	if ((err = ioctl(tap_fd, TUNSETIFF, (void *) &ifr)) < 0) {
		perror("ioctl(TUNSETIFF)");
		return 1;
	}

	/* CONNECT WITH SLICZ */

	slicz_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (slicz_fd < 0) syserr("socket");

	struct addrinfo addr_hints;
	struct addrinfo *address;
	memset(&addr_hints, 0, sizeof (struct addrinfo));

	addr_hints.ai_family = AF_INET;
	addr_hints.ai_socktype = SOCK_DGRAM;
	err = getaddrinfo(slicz_addr, slicz_port, &addr_hints, &address);
	if (err != 0) syserr("getaddrinfo");

	if (connect(slicz_fd, address->ai_addr, address->ai_addrlen) < 0)
		syserr("connect");

	freeaddrinfo(address);

	/* START EVENT LOOP */

	struct event_base *base = event_base_new();
	if (!base) syserr("Error creating base");

	struct event *slicz_event =
					event_new(base, slicz_fd, EV_READ | EV_PERSIST, slicz_read_callback, 0);
	event_add(slicz_event, NULL);
	struct event *tap_event =
					event_new(base, tap_fd, EV_READ | EV_PERSIST, tap_read_callback, 0);
	event_add(tap_event, NULL);

	event_base_dispatch(base);

	return 0;
}
