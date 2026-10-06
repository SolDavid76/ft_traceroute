#pragma once

#include "ft_traceroute.h"

/* Returns 1 for a reply, 0 for a timeout or stop, -1 for an error. */
int ft_traceroute_probe(t_traceroute *traceroute, t_probe_reply *reply);
