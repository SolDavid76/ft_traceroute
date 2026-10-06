#include "init.h"
#include "ft_traceroute.h"

#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

static int resolve_target(char *target, struct addrinfo **res) {
	struct addrinfo hints = {0};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	int ret = getaddrinfo(target, "33434", &hints, res);
	if (ret != 0) {
		fprintf(stderr, "ft_traceroute: %s: %s\n", target, gai_strerror(ret));
		return (1);
	}

	return (0);
}

static int open_icmp_socket(void) {
	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sock == -1) {
		perror("ft_traceroute: socket ICMP");
		return (-1);
	}

	return (sock);
}

static int open_udp_socket(void) {
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		perror("ft_traceroute: socket UDP");
		return (-1);
	}

	return (sock);
}

int ft_traceroute_init(t_traceroute *traceroute) {
	traceroute->target = NULL;
	traceroute->recv_sock = -1;
	traceroute->send_sock = -1;
	traceroute->ttl = 1;

	if (resolve_target(traceroute->option.target, &traceroute->target)) {
		return (1);
	}

	traceroute->recv_sock = open_icmp_socket();
	if (traceroute->recv_sock == -1) {
		return (1);
	}

	traceroute->send_sock = open_udp_socket();
	if (traceroute->send_sock == -1) {
		return (1);
	}

	if (setsockopt(
			traceroute->send_sock,
			IPPROTO_IP,
			IP_TTL,
			&traceroute->ttl,
			sizeof(traceroute->ttl)
		) == -1) {
		perror("ft_traceroute: setsockopt IP_TTL");
		return (1);
	}

	return (0);
}

void ft_traceroute_destroy(t_traceroute *traceroute) {
	if (traceroute->target != NULL) {
		freeaddrinfo(traceroute->target);
		traceroute->target = NULL;
	}
	if (traceroute->recv_sock != -1) {
		close(traceroute->recv_sock);
		traceroute->recv_sock = -1;
	}
	if (traceroute->send_sock != -1) {
		close(traceroute->send_sock);
		traceroute->send_sock = -1;
	}
}
