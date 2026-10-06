#pragma once

#include "ft_traceroute.h"

void ft_traceroute_print_help(void);
int ft_traceroute_print_header(const struct addrinfo* resolved_target);
void ft_traceroute_print_hop_start(int ttl);
int ft_traceroute_print_reply(t_probe_reply reply, int print_address);
void ft_traceroute_print_timeout(void);
void ft_traceroute_print_hop_end(void);
