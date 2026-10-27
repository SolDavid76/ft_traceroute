#pragma once

#include "ft_traceroute.h"

/* Decode the quoted ports; the caller matches them to a pending probe. */
/* Returns 1 for a matching packet, 0 to ignore it, -1 for an error. */
int ft_traceroute_receive_icmp_packet(int sock, const struct sockaddr_in *target, t_probe_reply *reply);
