#include "ft_traceroute.h"
#include "print.h"
#include "init.h"
#include "probe.h"

#include <getopt.h>
#include <signal.h>
#include <stdio.h>

volatile sig_atomic_t g_stop = 0;

static void signal_handler(int signal) {
	(void)signal;
	g_stop = 1;
}

int ft_traceroute(t_traceroute_option option) {
	if (option.help) {
		ft_traceroute_print_help();
		return (0);
	}

	t_traceroute traceroute = {0};

	traceroute.option = option;
	if (ft_traceroute_init(&traceroute)) {
		ft_traceroute_destroy(&traceroute);
		return (1);
	}

	if (ft_traceroute_print_header(traceroute.target)) {
		ft_traceroute_destroy(&traceroute);
		return (1);
	}

	while (traceroute.ttl <= TRACEROUTE_MAX_HOPS && !g_stop) {
		int probe_index = 0;
		int reached = 0;
		int have_previous = 0;
		struct in_addr previous = {0};

		ft_traceroute_print_hop_start(traceroute.ttl);
		while (probe_index < TRACEROUTE_PROBES_PER_HOP && !g_stop) {
			t_probe_reply reply = {0};
			int ret = ft_traceroute_probe(&traceroute, &reply);
			if (ret == -1) {
				ft_traceroute_print_hop_end();
				ft_traceroute_destroy(&traceroute);
				return (1);
			}

			if (g_stop) {
				break;
			}

			if (ret == 0) {
				ft_traceroute_print_timeout();
			} else {
				int print_address = !have_previous || previous.s_addr != reply.from.s_addr;
				if (ft_traceroute_print_reply(reply, print_address)) {
					ft_traceroute_print_hop_end();
					ft_traceroute_destroy(&traceroute);
					return (1);
				}

				previous = reply.from;
				have_previous = 1;
				reached = reached || reply.reached;
			}

			probe_index++;
		}
		ft_traceroute_print_hop_end();

		if (reached) {
			break;
		}

		traceroute.ttl++;
	}

	ft_traceroute_destroy(&traceroute);
	return (0);
}

static int parse_argument(int ac, char** av, t_traceroute_option* option) {
	struct option long_options[] = {
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0},
	};

	int opt;
	opterr = 0;
	while ((opt = getopt_long(ac, av, "", long_options, NULL)) != -1) {
		if (opt == 'h') {
			option->help = 1;
		} else {
			fprintf(stderr, "%s: invalid option or option argument\n", av[0]);
			return (-1);
		}
	}

	if (ac - optind > 1) {
		fprintf(stderr, "%s: too many host operands\n", av[0]);
		return (-1);
	}

	if (option->help) {
		return (0);
	}

	if (optind == ac) {
		fprintf(stderr, "%s: missing host operand\n", av[0]);
		return (-1);
	}

	option->target = av[optind];
	return (0);
}

int main(int ac, char** av) {
	signal(SIGINT, signal_handler);

	t_traceroute_option option = {0};
	if (parse_argument(ac, av, &option) == -1) {
		fprintf(stderr, "Try 'ft_traceroute --help' for more information.\n");
		return (1);
	}

	return (ft_traceroute(option));
}
