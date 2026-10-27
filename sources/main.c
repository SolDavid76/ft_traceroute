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

	if (ft_traceroute_print_header(traceroute.option.target, traceroute.target)) {
		ft_traceroute_destroy(&traceroute);
		return (1);
	}

	size_t sent_count = 0;
	size_t printed_count = 0;
	int status = 0;
	int line_open = 1;
	int have_previous = 0;
	struct in_addr previous = {0};

	ft_traceroute_print_hop_start(1);
	while (printed_count < traceroute.probe_limit && !g_stop) {
		if (ft_traceroute_send_probes(&traceroute, &sent_count) == -1) {
			status = 1;
			break;
		}

		while (printed_count < sent_count && printed_count < traceroute.probe_limit && !g_stop) {
			const t_probe *probe = &traceroute.probes[printed_count];
			if (probe->state != PROBE_REPLIED && probe->state != PROBE_TIMEOUT) {
				break;
			}

			if (!line_open) {
				ft_traceroute_print_hop_start(printed_count / TRACEROUTE_PROBES_PER_HOP + 1);
				line_open = 1;
				have_previous = 0;
			}

			if (probe->state == PROBE_TIMEOUT) {
				ft_traceroute_print_timeout();
			} else {
				int print_address = !have_previous || previous.s_addr != probe->reply.from.s_addr;
				if (ft_traceroute_print_reply(probe->reply, print_address)) {
					status = 1;
					break;
				}

				previous = probe->reply.from;
				have_previous = 1;
			}

			printed_count++;
			if (printed_count % TRACEROUTE_PROBES_PER_HOP == 0) {
				ft_traceroute_print_hop_end();
				line_open = 0;
			}
		}

		if (status || g_stop || printed_count == traceroute.probe_limit) {
			break;
		}

		if (ft_traceroute_wait_reply(&traceroute, sent_count) == -1) {
			status = 1;
			break;
		}
	}

	if (line_open) {
		ft_traceroute_print_hop_end();
	}
	ft_traceroute_destroy(&traceroute);
	return (status);
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
