#include "probe.h"
#include "receive.h"

#include <errno.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

static ssize_t send_probe(int sock, struct addrinfo *target) {
	char payload[] = "probe";

	ssize_t sent = sendto(
		sock,
		payload,
		sizeof(payload) - 1,
		0,
		target->ai_addr,
		target->ai_addrlen
	);
	if (sent == -1) {
		perror("ft_traceroute: sendto");
	}

	return (sent);
}

static int get_probe_timeout(t_traceroute *traceroute, struct timeval *timeout) {
	struct timespec now;
	if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
		perror("ft_traceroute: clock_gettime");
		return (-1);
	}

	double elapsed = (now.tv_sec - traceroute->sent_at.tv_sec)
		+ (now.tv_nsec - traceroute->sent_at.tv_nsec) / 1000000000.0;
	double remaining = TRACEROUTE_WAIT_SECONDS - elapsed;
	if (remaining <= 0) {
		return (0);
	}

	timeout->tv_sec = (time_t)remaining;
	timeout->tv_usec = (suseconds_t)((remaining - timeout->tv_sec) * 1000000);
	return (timeout->tv_sec != 0 || timeout->tv_usec != 0);
}

static int wait_for_reply(t_traceroute *traceroute, t_probe_reply *reply) {
	if (traceroute->recv_sock < 0 || traceroute->recv_sock >= FD_SETSIZE) {
		fprintf(stderr, "ft_traceroute: socket descriptor out of range for select\n");
		return (-1);
	}

	while (!g_stop) {
		struct timeval timeout;
		int ret = get_probe_timeout(traceroute, &timeout);
		if (ret <= 0) {
			return (ret);
		}

		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(traceroute->recv_sock, &readfds);
		int ready = select(traceroute->recv_sock + 1, &readfds, NULL, NULL, &timeout);
		if (ready == -1) {
			if (errno == EINTR) {
				continue;
			}

			perror("ft_traceroute: select");
			return (-1);
		}

		if (ready == 0) {
			return (0);
		}

		if (g_stop) {
			break;
		}

		if (!FD_ISSET(traceroute->recv_sock, &readfds)) {
			continue;
		}

		/* Bound recvfrom too, even if select reports readiness spuriously. */
		ret = get_probe_timeout(traceroute, &timeout);
		if (ret <= 0) {
			return (ret);
		}

		if (setsockopt(traceroute->recv_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
			perror("ft_traceroute: setsockopt SO_RCVTIMEO");
			return (-1);
		}

		ret = receive_icmp_packet(traceroute, reply);
		if (ret == -1) {
			return (-1);
		}

		if (ret == 0) {
			continue;
		}

		struct timespec now;
		if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
			perror("ft_traceroute: clock_gettime");
			return (-1);
		}

		reply->rtt = (now.tv_sec - traceroute->sent_at.tv_sec) * 1000.0;
		reply->rtt += (now.tv_nsec - traceroute->sent_at.tv_nsec) / 1000000.0;
		if (reply->rtt > TRACEROUTE_WAIT_SECONDS * 1000.0) {
			return (0);
		}

		return (1);
	}

	return (0);
}

int ft_traceroute_probe(t_traceroute *traceroute, t_probe_reply *reply) {
	if (g_stop) {
		return (0);
	}

	if (clock_gettime(CLOCK_MONOTONIC, &traceroute->sent_at) == -1) {
		perror("ft_traceroute: clock_gettime");
		return (-1);
	}

	if (send_probe(traceroute->send_sock, traceroute->target) == -1) {
		return (-1);
	}

	struct sockaddr_in source = {0};
	socklen_t source_len = sizeof(source);
	if (getsockname(traceroute->send_sock, (struct sockaddr *)&source, &source_len) == -1) {
		perror("ft_traceroute: getsockname");
		return (-1);
	}

	traceroute->source_port = source.sin_port;

	return (wait_for_reply(traceroute, reply));
}
