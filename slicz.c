#include <event2/util.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>

#include <arpa/inet.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "error_codes.h"
#include "err.h"
#include "ports.h"
#include "utils.h"
#include "slicz.h"

#define DEFAULT_CONTROL_PORT 42420
#define MAX_CONNECTIONS_NUMBER 20

#define SETCONFIG_COMMAND "setconfig"
#define GETCONFIG_COMMAND "getconfig"
#define COUNTERS_COMMAND "counters"

int active_connections = 0;
int control_port;
struct event_base *base;

struct event_base* get_base() {
	return base;
}

/* prints error to given buffer */
static void print_error_to_buf(char* err_info, struct evbuffer* output) {
	evbuffer_add_printf(output, "ERR %s\n", err_info);
}

/* processes setconfig, prints result to given buffer */
static void process_setconfig(char* config, struct evbuffer* output) {
	int err = setconfig(config);
	if (err != OK)
		print_error_to_buf(get_error_message(err), output);
	else
		evbuffer_add_printf(output, "OK\n");
}

/* processes getconfig, prints result to given buffer */
static void process_getconfig(struct evbuffer* output) {
	port_list_t* iterator = port_list_get_first();
	char buf[MAX_COMMAND_LINE_LENGTH];
	
	evbuffer_add_printf(output, "OK\n");
	while (iterator != NULL) {
		print_port_description(buf, iterator);
		evbuffer_add_printf(output, "%s\n", buf);
		iterator = port_list_get_next(iterator);
	}
	evbuffer_add_printf(output, "END\n");
}

/* processes counters, prints result to given buffer */
static void process_counters(struct evbuffer* output) {
	port_list_t* iterator = port_list_get_first();
	uint16_t port;
	unsigned recv, sent, errs;
	evbuffer_add_printf(output, "OK\n");
	while (iterator != NULL) {
		get_port_counters(iterator, &port, &recv, &sent, &errs);
		evbuffer_add_printf(output, "%d recvd:%d sent:%d errs:%d\n", 
						port, recv, sent, errs);
		iterator = port_list_get_next(iterator);
	}
	evbuffer_add_printf(output, "END\n");
}

/* parses command, prints output to given buffer */
static void process_command(char* command_line, struct evbuffer* output) {
	printf("process command [%s]\n", command_line);
	if (starts_with(command_line, SETCONFIG_COMMAND))
		process_setconfig(command_line + strlen(SETCONFIG_COMMAND) + 1, output);
		/* + 1 for space */
	else if (starts_with(command_line, GETCONFIG_COMMAND))
		process_getconfig(output);
	else if (starts_with(command_line, COUNTERS_COMMAND))
		process_counters(output);
	else
		print_error_to_buf("Unknown command", output);
}

/* this callback is invoked when there is data to read on bev. */
static void control_read_cb(struct bufferevent *bev, void *ctx) {

	struct evbuffer *input = bufferevent_get_input(bev);
	struct evbuffer *output = bufferevent_get_output(bev);

	char *command_line;
	size_t len;

	/* read one line ended with '\n' from buffer */
	command_line = evbuffer_readln(input, &len, EVBUFFER_EOL_LF);

	if (command_line) { /* in other case first line didn't arrive yet*/
		if (len > MAX_COMMAND_LINE_LENGTH)
			print_error_to_buf("Command line too long", output);
		else
			process_command(command_line, output);
		free(command_line);
	}
	
}

/* on special event */
static void control_event_cb(struct bufferevent *bev, short events, void *ctx) {
	if (events & BEV_EVENT_ERROR)
		perror("Error from bufferevent");
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		bufferevent_free(bev);
		active_connections--;
	}
}

/* on new control connection */
static void accept_conn_cb(struct evconnlistener *listener,
				evutil_socket_t fd, struct sockaddr *address, int socklen,
				void *ctx) {
	
	active_connections++;
	struct event_base *base = evconnlistener_get_base(listener);
	struct bufferevent *bev = bufferevent_socket_new(
					base, fd, BEV_OPT_CLOSE_ON_FREE);

	bufferevent_setcb(bev, control_read_cb, NULL, control_event_cb, NULL);

	bufferevent_enable(bev, EV_READ | EV_WRITE);
	
	struct evbuffer *output = bufferevent_get_output(bev);
	if (active_connections > MAX_CONNECTIONS_NUMBER) {
		print_error_to_buf("Too many connections, rejecting", output);
		close(fd);
		bufferevent_free(bev);
		active_connections--;
	} else {
		evbuffer_add_printf(bufferevent_get_output(bev), "SLICZ\n");
	}
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx) {
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
					"Shutting down.\n", err, evutil_socket_error_to_string(err));

	event_base_loopexit(base, NULL);
}

void slicz_start() {
	base = event_base_new();
	if (!base) syserr("Error creating base");

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(control_port);

	/* basing on example echo server from libevent documentation */
	struct evconnlistener *listener;
	listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
					LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
					(struct sockaddr*) &sin, sizeof (sin));
	if (!listener)
		syserr("Creating TCP listener listener");

	evconnlistener_set_error_cb(listener, accept_error_cb);
	
	event_base_dispatch(base);
}

int main(int argc, char** argv) {
	char opt;
	control_port = DEFAULT_CONTROL_PORT;
	int was_c = 0;
	while ((opt = getopt(argc, argv, ":c:p")) != -1) {
		switch (opt) {
			case 'c':
				if (was_c == 1)
					fatal("There should be one 'c' parametr.");
				was_c = 1;
				control_port = atoi(optarg);
				break;
			case 'p':
				if (setconfig(optarg) != 0)
					fatal("Invalid configuration: %s", optarg);
				break;
			default:
				fatal("Wrong parameter");
		}
	}

	printf("starting\n");
	slicz_start();
}