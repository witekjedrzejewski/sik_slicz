#include <stdio.h>
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

#define MAX_FRAME_SIZE 1518
#define MAX_TAP_NAME_LENGTH 50
#define DEFAULT_TAP_NAME "siktap"

int tap_fd;
int slicz_fd;
char buf[MAX_FRAME_SIZE];
char tap_name[MAX_TAP_NAME_LENGTH];

void slicz_read_callback(evutil_socket_t sock, short ev, void* arg) {
  int r = read(slicz_fd, buf, sizeof(buf));
  if (r < 0) syserr("Slicz socket read");
  
	int w = write(tap_fd, buf, r);
  if (w < 0) syserr("Tap socket write");
}

void tap_read_callback(evutil_socket_t sock, short ev, void* arg) {
  int r = read(tap_fd, buf, sizeof(buf));
  if (r < 0) syserr("Tap socket read");
  
	int w = write(slicz_fd, buf, r);
  if (w < 0) syserr("Slicz socket write");
}

int main(int argc, char** argv) {
	
	char c;
	
	strcpy(tap_name, DEFAULT_TAP_NAME);
	while ((c = getopt(argc, argv, "d:s:")) != -1) {
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

	/* TODO parsing */
	
	/* INIT TAP */
	
  struct ifreq ifr;
  int err;

  if ((tap_fd = open("/dev/net/tun", O_RDWR)) < 0) {
    perror("open(/dev/net/tun)");
    return 1;
  }

  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, "siktap", IFNAMSIZ);

  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  if ((err = ioctl(tap_fd, TUNSETIFF, (void *) &ifr)) < 0) {
    perror("ioctl(TUNSETIFF)");
    return 1;
  }

	/* CONNECT WITH SLICZ */
	
	/* START EVENT LOOP */
	
  struct event_base *base = event_base_new();
	if (!base) syserr("Error creating base");
	
  struct event *slicz_event = 
    event_new(base, slicz_fd, EV_READ|EV_PERSIST, slicz_read_callback, 0);
  event_add(slicz_event, NULL);
  struct event *tap_event = 
    event_new(base, tap_fd, EV_READ|EV_PERSIST, tap_read_callback, 0);
  event_add(tap_event, NULL);
  
	event_base_dispatch(base);
  
	return 0;
}
