#include "print.h"
#include "ft_traceroute.h"

#include <arpa/inet.h>
#include <stdio.h>

void ft_traceroute_print_help(void) {
	printf("Usage: ft_traceroute [OPTION...] HOST\n");
	printf("Trace the route to an IPv4 host.\n\n");
	printf("  --help                      Read this help and exit\n");
}

int ft_traceroute_print_header(struct addrinfo* resolved_target) {
	struct sockaddr_in *addr = (struct sockaddr_in *)resolved_target->ai_addr;
	char ip[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip)) == NULL) {
		perror("ft_traceroute: inet_ntop");
		return (1);
	}

	printf("Destination : %s\n", ip);
	return (0);
}

int ft_traceroute_print_reply(int ttl, t_probe_reply reply) {
	char ip[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &reply.from, ip, sizeof(ip)) == NULL) {
		perror("ft_traceroute: inet_ntop");
		return (1);
	}

	printf("%2d  %s  %.3f ms%s\n", ttl, ip, reply.rtt, reply.reached ? " (destination)" : "");
	return (0);
}

void ft_traceroute_print_timeout(int ttl) {
	printf("%2d  *\n", ttl);
}
