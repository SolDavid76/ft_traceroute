#pragma once

#include "ft_traceroute.h"

/* Fill the available slots, keeping at most TRACEROUTE_SIM_PROBES pending. */
int ft_traceroute_send_probes(t_traceroute *traceroute, size_t *sent_count);
/* Receive one response or expire probes; returns -1 on error, 0 otherwise. */
int ft_traceroute_wait_reply(t_traceroute *traceroute, size_t sent_count);
