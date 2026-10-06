#pragma once

#include "ft_traceroute.h"

void ft_traceroute_print_help(void);
int ft_traceroute_print_header(struct addrinfo* resolved_target);
int ft_traceroute_print_reply(int ttl, t_probe_reply reply);
void ft_traceroute_print_timeout(int ttl);
