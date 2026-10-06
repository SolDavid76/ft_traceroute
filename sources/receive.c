#include "receive.h"

#include <errno.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

int ft_traceroute_receive_icmp_packet(int sock, const struct sockaddr_in *target, uint16_t source_port, t_probe_reply *reply) {
	unsigned char buffer[2048];
	struct sockaddr_in from = {0};
	socklen_t from_len = sizeof(from);
	ssize_t received = recvfrom(
		sock,
		buffer,
		sizeof(buffer),
		0,
		(struct sockaddr *)&from,
		&from_len
	);
	if (received == -1) {
		if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
			return (0);
		}

		perror("ft_traceroute: recvfrom");
		return (-1);
	}

	/* Read the IPv4 header of the reply. */
	if (received < (ssize_t)sizeof(struct iphdr)) {
		return (0);
	}

	struct iphdr ip;
	memcpy(&ip, buffer, sizeof(ip));

	size_t ip_len = ip.ihl * 4;
	size_t packet_len = ntohs(ip.tot_len);
	if (ip.version != 4 || ip.ihl < 5 || ip.protocol != IPPROTO_ICMP) {
		return (0);
	}

	if (packet_len > (size_t)received || packet_len < ip_len + sizeof(struct icmphdr)) {
		return (0);
	}

	/* Accept TTL expiration or an unreachable destination port. */
	struct icmphdr icmp;
	memcpy(&icmp, buffer + ip_len, sizeof(icmp));

	int reached = icmp.type == ICMP_DEST_UNREACH && icmp.code == ICMP_PORT_UNREACH;
	if (!reached && !(icmp.type == ICMP_TIME_EXCEEDED && icmp.code == ICMP_EXC_TTL)) {
		return (0);
	}

	/* The ICMP message contains the original IPv4 and UDP headers. */
	size_t inner_offset = ip_len + sizeof(icmp);
	if (packet_len < inner_offset + sizeof(struct iphdr)) {
		return (0);
	}

	struct iphdr inner_ip;
	memcpy(&inner_ip, buffer + inner_offset, sizeof(inner_ip));

	size_t inner_len = inner_ip.ihl * 4;
	if (inner_ip.version != 4 || inner_ip.ihl < 5 || inner_ip.protocol != IPPROTO_UDP) {
		return (0);
	}

	if ((ntohs(inner_ip.frag_off) & IP_OFFMASK) != 0) {
		return (0);
	}

	if (ntohs(inner_ip.tot_len) < inner_len + sizeof(struct udphdr)) {
		return (0);
	}

	if (packet_len < inner_offset + inner_len + sizeof(struct udphdr)) {
		return (0);
	}

	/* Match the quoted destination and ports against our probe. */
	if (inner_ip.daddr != target->sin_addr.s_addr) {
		return (0);
	}

	struct udphdr udp;
	memcpy(&udp, buffer + inner_offset + inner_len, sizeof(udp));
	if (udp.source != source_port || udp.dest != target->sin_port) {
		return (0);
	}

	if (ntohs(udp.len) < sizeof(udp)) {
		return (0);
	}

	if (reached && from.sin_addr.s_addr != target->sin_addr.s_addr) {
		return (0);
	}

	reply->from = from.sin_addr;
	reply->reached = reached;
	return (1);
}
