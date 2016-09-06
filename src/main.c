#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "log.h"

static struct {
	int running;
} global = {
	.running = 1,
};

static void handler(int signal)
{
	switch (signal) {
	case SIGHUP:
		info("Daemon is alive");
		break;

	case SIGINT:
	case SIGQUIT:
	case SIGTERM:
		info("Stopping daemon");

		global.running = 0;
		break;
	}
}

int main(int argc, char **argv)
{
	int opt;

	const struct option lopt[] = {
		{"version",	no_argument, NULL, 'v'},
		{"help",	no_argument, NULL, 'h'},
		{NULL,		0, NULL, 0},
	};

	while ((opt = getopt_long(argc, argv, "vh", lopt, NULL)) != EOF) {
		switch (opt) {
		case 'v':
			fprintf(stdout, "daemon %s\n", VERSION);
			exit(EXIT_SUCCESS);

		default:
			fprintf(stderr, "Usage: daemon [OPTIONS]...\n"
			        "-v, --version     Display version.\n"
			        "-h, --help        Display this help screen.\n");
			exit(EXIT_FAILURE);
		}
	}

	openlog("daemon", LOG_PID, LOG_DAEMON);
	info("Starting daemon");

	if (daemon(0, 0) < 0) {
		ecritical("Failed to daemonize");
		exit(EXIT_FAILURE);

	} else if (chdir("/") < 0) {
		ecritical("Failed to chdir to /");
		exit(EXIT_FAILURE);

	} else if (signal(SIGHUP, handler) == SIG_ERR) {
		ecritical("Failed to handle SIGHUP");
		exit(EXIT_FAILURE);

	} else if (signal(SIGINT, handler) == SIG_ERR) {
		ecritical("Failed to handle SIGINT");
		exit(EXIT_FAILURE);

	} else if (signal(SIGQUIT, handler) == SIG_ERR) {
		ecritical("Failed to handle SIGQUIT");
		exit(EXIT_FAILURE);

	} else if (signal(SIGTERM, handler) == SIG_ERR) {
		ecritical("Failed to handle SIGTERM");
		exit(EXIT_FAILURE);
	}

	while (global.running == 1) {
		sleep(1);
	}

	closelog();

	exit(EXIT_SUCCESS);
}
