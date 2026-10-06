#include "print.h"
#include "ft_traceroute.h"

#include <arpa/inet.h>
#include <stdio.h>

void ft_traceroute_print_help(void) {
	printf("Usage: ft_traceroute [OPTION...] HOST\n");
	printf("Trace the route to an IPv4 host.\n\n");
	printf("  --help                      Read this help and exit\n");
}

int ft_traceroute_print_header(const struct addrinfo* resolved_target) {
	const struct sockaddr_in *addr = (const struct sockaddr_in *)resolved_target->ai_addr;
	char ip[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip)) == NULL) {
		perror("ft_traceroute: inet_ntop");
		return (1);
	}

	printf("Destination : %s\n", ip);
	return (0);
}

void ft_traceroute_print_hop_start(int ttl) {
	printf("%2d", ttl);
	fflush(stdout);
}

int ft_traceroute_print_reply(t_probe_reply reply, int print_address) {
	if (print_address) {
		char ip[INET_ADDRSTRLEN];
		if (inet_ntop(AF_INET, &reply.from, ip, sizeof(ip)) == NULL) {
			perror("ft_traceroute: inet_ntop");
			return (1);
		}

		printf("  %s", ip);
	}

	printf("  %.3f ms", reply.rtt);
	fflush(stdout);
	return (0);
}

void ft_traceroute_print_timeout(void) {
	printf("  *");
	fflush(stdout);
}

void ft_traceroute_print_hop_end(void) {
	printf("\n");
	fflush(stdout);
}
