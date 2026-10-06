#pragma once

#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>

#define TRACEROUTE_WAIT_SECONDS 3

extern volatile sig_atomic_t g_stop;

typedef struct s_traceroute_option {
	int help;
	char* target;
} t_traceroute_option;

typedef struct s_traceroute {
	t_traceroute_option option;
	struct addrinfo *target;
	int recv_sock;
	int send_sock;
	int ttl;
	uint16_t source_port;
	struct timespec sent_at;
} t_traceroute;

typedef struct s_probe_reply {
	struct in_addr from;
	double rtt;
	int reached;
} t_probe_reply;
