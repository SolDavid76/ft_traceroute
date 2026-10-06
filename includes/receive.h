#pragma once

#include "ft_traceroute.h"

/* Returns 1 for our probe, 0 for an unrelated packet, -1 for an error. */
int receive_icmp_packet(t_traceroute *traceroute, t_probe_reply *reply);
