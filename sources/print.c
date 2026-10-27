#include "print.h"
#include "ft_traceroute.h"

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <stdio.h>

static void print_unreachable(uint8_t code, uint16_t next_hop_mtu) {
	switch (code) {
		case ICMP_NET_UNREACH:
		case ICMP_NET_UNKNOWN:
		case ICMP_HOST_ISOLATED:
		case ICMP_NET_UNR_TOS:
			printf(" !N");
			break;
		case ICMP_HOST_UNREACH:
		case ICMP_HOST_UNKNOWN:
		case ICMP_HOST_UNR_TOS:
			printf(" !H");
			break;
		case ICMP_PROT_UNREACH:
			printf(" !P");
			break;
		case ICMP_FRAG_NEEDED:
			printf(" !F-%u", (unsigned int)next_hop_mtu);
			break;
		case ICMP_SR_FAILED:
			printf(" !S");
			break;
		case ICMP_NET_ANO:
		case ICMP_HOST_ANO:
		case ICMP_PKT_FILTERED:
			printf(" !X");
			break;
		case ICMP_PREC_VIOLATION:
			printf(" !V");
			break;
		case ICMP_PREC_CUTOFF:
			printf(" !C");
			break;
		default:
			printf(" !<%u>", (unsigned int)code);
			break;
	}
}

void ft_traceroute_print_help(void) {
	printf("Usage:\n");
	printf("  ft_traceroute [OPTION...] host\n\n");
	printf("Options:\n");
	printf("  --help                      Read this help and exit\n\n");
	printf("Arguments:\n");
	printf("      host          The host to traceroute to\n");
}

int ft_traceroute_print_header(const char* target, const struct addrinfo* resolved_target) {
	const struct sockaddr_in *addr = (const struct sockaddr_in *)resolved_target->ai_addr;
	char ip[INET_ADDRSTRLEN];
	if (inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip)) == NULL) {
		perror("ft_traceroute: inet_ntop");
		return (1);
	}

	size_t packet_size = sizeof(struct iphdr) + sizeof(struct udphdr) + TRACEROUTE_PAYLOAD_SIZE;
	printf("traceroute to %s (%s), %d hops max, %zu byte packets\n",
		target, ip, TRACEROUTE_MAX_HOPS, packet_size
	);
	return (0);
}

void ft_traceroute_print_hop_start(int ttl) {
	printf("%2d ", ttl);
	fflush(stdout);
}

int ft_traceroute_print_reply(t_probe_reply reply, int print_address) {
	if (print_address) {
		char ip[INET_ADDRSTRLEN];
		if (inet_ntop(AF_INET, &reply.from, ip, sizeof(ip)) == NULL) {
			perror("ft_traceroute: inet_ntop");
			return (1);
		}

		printf(" %s", ip);
	}

	printf("  %.3f ms", reply.rtt);
	if (reply.unreachable) {
		print_unreachable(reply.icmp_code, reply.next_hop_mtu);
	}
	fflush(stdout);
	return (0);
}

void ft_traceroute_print_timeout(void) {
	printf(" *");
	fflush(stdout);
}

void ft_traceroute_print_hop_end(void) {
	printf("\n");
	fflush(stdout);
}
