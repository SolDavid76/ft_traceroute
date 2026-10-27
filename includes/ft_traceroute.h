#pragma once

#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stddef.h>
#include <signal.h>
#include <time.h>

#define TRACEROUTE_WAIT_SECONDS 5
#define TRACEROUTE_MAX_HOPS 30
#define TRACEROUTE_BASE_PORT 33434
#define TRACEROUTE_PROBES_PER_HOP 3
#define TRACEROUTE_PAYLOAD_SIZE 32
#define TRACEROUTE_SIM_PROBES 16
#define TRACEROUTE_MAX_PROBES (TRACEROUTE_MAX_HOPS * TRACEROUTE_PROBES_PER_HOP)

extern volatile sig_atomic_t g_stop;

typedef struct s_traceroute_option {
	int help;
	char* target;
} t_traceroute_option;

typedef struct s_probe_reply {
	struct in_addr from;
	double rtt;
	int reached;
	int unreachable;
	uint8_t icmp_code;
	uint16_t next_hop_mtu;
	uint16_t dest_port;
	uint16_t source_port;
} t_probe_reply;

typedef enum e_probe_state {
	PROBE_UNSENT,
	PROBE_PENDING,
	PROBE_REPLIED,
	PROBE_TIMEOUT
} t_probe_state;

typedef struct s_probe {
	t_probe_state state;
	int send_sock;
	uint16_t source_port;
	struct timespec sent_at;
	t_probe_reply reply;
} t_probe;

typedef struct s_traceroute {
	t_traceroute_option option;
	struct addrinfo *target;
	int recv_sock;
	size_t probe_limit;
	t_probe probes[TRACEROUTE_MAX_PROBES];
} t_traceroute;
