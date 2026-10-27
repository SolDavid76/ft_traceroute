#include "probe.h"
#include "receive.h"

#include <errno.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int open_probe_socket(uint16_t *source_port) {
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1) {
		perror("ft_traceroute: socket UDP");
		return (-1);
	}

	struct sockaddr_in source = {0};
	source.sin_family = AF_INET;
	if (bind(sock, (struct sockaddr *)&source, sizeof(source)) == -1) {
		perror("ft_traceroute: bind UDP");
		close(sock);
		return (-1);
	}

	socklen_t source_len = sizeof(source);
	if (getsockname(sock, (struct sockaddr *)&source, &source_len) == -1) {
		perror("ft_traceroute: getsockname");
		close(sock);
		return (-1);
	}

	*source_port = source.sin_port;
	return (sock);
}

static void complete_probe(t_probe *probe, t_probe_state state) {
	if (probe->send_sock != -1) {
		close(probe->send_sock);
		probe->send_sock = -1;
	}
	probe->state = state;
}

static int prepare_probe(int sock, int ttl, uint16_t seq, struct sockaddr_in *target) {
	if (setsockopt(
			sock,
			IPPROTO_IP,
			IP_TTL,
			&ttl,
			sizeof(ttl)
		) == -1) {
		perror("ft_traceroute: setsockopt IP_TTL");
		return (-1);
	}

	/* A different port per probe prevents matching replies to earlier probes. */
	target->sin_port = htons(TRACEROUTE_BASE_PORT + seq);
	return (0);
}

static ssize_t send_probe(int sock, const struct sockaddr_in *target) {
	unsigned char payload[TRACEROUTE_PAYLOAD_SIZE] = {0};

	ssize_t sent = sendto(
		sock,
		payload,
		sizeof(payload),
		0,
		(const struct sockaddr *)target,
		sizeof(*target)
	);
	if (sent == -1) {
		perror("ft_traceroute: sendto");
	}

	return (sent);
}

static double elapsed_seconds(struct timespec sent_at, struct timespec now) {
	return ((now.tv_sec - sent_at.tv_sec)
		+ (now.tv_nsec - sent_at.tv_nsec) / 1000000000.0);
}

static int get_wait_timeout(t_probe *probes, size_t count, struct timeval *timeout) {
	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
		perror("ft_traceroute: clock_gettime");
		return (-1);
	}

	double nearest = TRACEROUTE_WAIT_SECONDS;
	int pending = 0;
	int expired = 0;
	for (size_t i = 0; i < count; i++) {
		if (probes[i].state != PROBE_PENDING) {
			continue;
		}

		double remaining = TRACEROUTE_WAIT_SECONDS - elapsed_seconds(probes[i].sent_at, now);
		if (remaining <= 0) {
			complete_probe(&probes[i], PROBE_TIMEOUT);
			expired = 1;
			continue;
		}

		pending = 1;
		if (remaining < nearest) {
			nearest = remaining;
		}
	}

	timeout->tv_sec = (time_t)nearest;
	timeout->tv_usec = (suseconds_t)((nearest - timeout->tv_sec) * 1000000);
	/* A zero SO_RCVTIMEO would disable the receive timeout. */
	if (timeout->tv_sec == 0 && timeout->tv_usec == 0) {
		timeout->tv_usec = 1;
	}
	/* Let the scheduler print expired probes and refill their slots first. */
	return (expired ? 0 : pending);
}

static int record_reply(t_traceroute *traceroute, size_t sent_count, t_probe_reply reply) {
	int port = ntohs(reply.dest_port);
	if (port < TRACEROUTE_BASE_PORT) {
		return (0);
	}

	size_t index = port - TRACEROUTE_BASE_PORT;
	if (index >= sent_count || index >= traceroute->probe_limit) {
		return (0);
	}

	t_probe *probe = &traceroute->probes[index];
	if (probe->state != PROBE_PENDING || reply.source_port != probe->source_port) {
		return (0);
	}

	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
		perror("ft_traceroute: clock_gettime");
		return (-1);
	}

	double elapsed = elapsed_seconds(probe->sent_at, now);
	if (elapsed >= TRACEROUTE_WAIT_SECONDS) {
		complete_probe(probe, PROBE_TIMEOUT);
		return (0);
	}

	reply.rtt = elapsed * 1000.0;
	probe->reply = reply;
	complete_probe(probe, PROBE_REPLIED);
	if (reply.reached || reply.unreachable) {
		size_t limit = (index / TRACEROUTE_PROBES_PER_HOP + 1) * TRACEROUTE_PROBES_PER_HOP;
		if (limit < traceroute->probe_limit) {
			traceroute->probe_limit = limit;
			for (size_t i = limit; i < sent_count; i++) {
				if (traceroute->probes[i].state == PROBE_PENDING) {
					complete_probe(&traceroute->probes[i], PROBE_TIMEOUT);
				}
			}
		}
	}
	return (0);
}

int ft_traceroute_send_probes(t_traceroute *traceroute, size_t *sent_count) {
	int pending = 0;
	for (size_t i = 0; i < *sent_count && i < traceroute->probe_limit; i++) {
		pending += traceroute->probes[i].state == PROBE_PENDING;
	}

	while (*sent_count < traceroute->probe_limit && pending < TRACEROUTE_SIM_PROBES && !g_stop) {
		size_t index = *sent_count;
		int ttl = index / TRACEROUTE_PROBES_PER_HOP + 1;
		/* Keep the resolved destination unchanged while probes are in flight. */
		struct sockaddr_in target = *(const struct sockaddr_in *)traceroute->target->ai_addr;
		t_probe *probe = &traceroute->probes[index];
		/* Give each pending probe its own flow, including through a NAT. */
		probe->send_sock = open_probe_socket(&probe->source_port);
		if (probe->send_sock == -1) {
			return (-1);
		}

		if (prepare_probe(probe->send_sock, ttl, index, &target) == -1) {
			return (-1);
		}

		if (clock_gettime(CLOCK_MONOTONIC, &probe->sent_at) == -1) {
			perror("ft_traceroute: clock_gettime");
			return (-1);
		}

		if (send_probe(probe->send_sock, &target) == -1) {
			return (-1);
		}

		probe->state = PROBE_PENDING;
		(*sent_count)++;
		pending++;
	}
	return (0);
}

int ft_traceroute_wait_reply(t_traceroute *traceroute, size_t sent_count) {
	if (traceroute->recv_sock < 0 || traceroute->recv_sock >= FD_SETSIZE) {
		fprintf(stderr, "ft_traceroute: socket descriptor out of range for select\n");
		return (-1);
	}

	if (sent_count > traceroute->probe_limit) {
		sent_count = traceroute->probe_limit;
	}

	struct timeval timeout;
	int ret = get_wait_timeout(traceroute->probes, sent_count, &timeout);
	if (ret <= 0 || g_stop) {
		return (ret < 0 ? -1 : 0);
	}

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(traceroute->recv_sock, &readfds);
	int ready = select(traceroute->recv_sock + 1, &readfds, NULL, NULL, &timeout);
	if (ready == -1) {
		if (errno == EINTR) {
			return (0);
		}

		perror("ft_traceroute: select");
		return (-1);
	}

	/* Refresh deadlines after select, including when it times out. */
	ret = get_wait_timeout(traceroute->probes, sent_count, &timeout);
	if (ret <= 0 || g_stop || ready == 0 || !FD_ISSET(traceroute->recv_sock, &readfds)) {
		return (ret < 0 ? -1 : 0);
	}

	if (setsockopt(traceroute->recv_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
		perror("ft_traceroute: setsockopt SO_RCVTIMEO");
		return (-1);
	}

	t_probe_reply reply = {0};
	const struct sockaddr_in *target = (const struct sockaddr_in *)traceroute->target->ai_addr;
	ret = ft_traceroute_receive_icmp_packet(traceroute->recv_sock, target, &reply);
	if (ret <= 0) {
		return (ret);
	}

	return (record_reply(traceroute, sent_count, reply));
}
