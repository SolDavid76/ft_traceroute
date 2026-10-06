#pragma once

#include "ft_traceroute.h"

/* Returns 1 for our probe, 0 for an unrelated packet, -1 for an error. */
int ft_traceroute_receive_icmp_packet(int sock, const struct sockaddr_in *target, uint16_t source_port, t_probe_reply *reply);
