#include <arpa/inet.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "log.h"
#include "network.h"

#define PING_DELAY		1

static struct {
	int running;
} global = {
	.running = 1,
};

static void usage(void)
{
	fprintf(stderr,
	        "Usage: " BIN_NAME " [OPTIONS]...\n"
	        "The network ping pong tool.\n"
	        "\nListener mode:\n"
	        " -i, --interface IFACE   Use the specified interface"
	        " (default is " DEFAULT_IFACE ").\n"
	        "\nTalker mode:\n"
	        " -h, --host HOST         Use the specified host.\n"
	        "\nCommon options:\n"
	        " -p, --port PORT         Use the specified port"
	        " (default is " DEFAULT_PORT ").\n"
	        " -f, --foreground        Do not daemonize.\n"
	        "     --version           Display version.\n"
	        "     --help              Display this help screen.\n"
	        "\n" BIN_NAME " is part of the " PACKAGE_NAME " package.\n"
	        "Report bugs to <" PACKAGE_BUGREPORT ">.\n"
	        PACKAGE_NAME " home page: <" PACKAGE_URL ">.\n");
}

static void handler(int signal)
{
	switch (signal) {
	case SIGHUP:
		info(BIN_NAME " is alive");
		break;

	case SIGINT:
	case SIGQUIT:
	case SIGTERM:
		info("Stopping " BIN_NAME);

		global.running = 0;
		break;
	}
}

enum pp_type {
	PING = 1,
	PONG = 2,
};

struct pp_data {
	enum pp_type type;
	uint16_t id;
};

static int run_listener(const char *iface, const char *port)
{
	struct sockaddr_storage addr;
	struct pp_data data;
	socklen_t len;
	ssize_t count;
	int sockfd;

	if ((sockfd = get_listener(iface, port)) < 0) {
		error("Failed to get listener socket");
		return -1;
	}

	while (global.running == 1) {
		memset(&data, 0, sizeof(struct pp_data));
		len = sizeof(struct sockaddr_storage);

		if ((count = recv_from(sockfd, &data, sizeof(struct pp_data),
		                       (struct sockaddr *)&addr, &len)) < 0) {
			warning("Failed to receive request");
			continue;

		} else if (count == 0) {
			continue;

		} else if (data.type != PING) {
			warning("Invalid data received");
			continue;

		} else {
			notice("Got  ping (%u)", ntohs(data.id));
		}

		data.type = PONG;

		if ((count = send_to(sockfd, &data, sizeof(struct pp_data),
		                     (struct sockaddr *)&addr, len)) < 0) {
			warning("Failed to send response");

		} else if (count == 0) {
			warning("Nothing sent");

		} else {
			notice("Sent pong (%u)", ntohs(data.id));
		}
	}

	close(sockfd);
	return 0;
}

static int run_talker(const char *host, const char *port)
{
	struct sockaddr_storage addr;
	struct pp_data data;
	socklen_t len;
	ssize_t count;
	int sockfd;
	uint16_t id;

	if ((sockfd = get_talker(host, port,
	                         (struct sockaddr *)&addr, &len)) < 0) {
		error("Failed to get talker socket");
		return -1;
	}

	while (global.running == 1) {
		id = rand();
		data.type = PING;
		data.id = htons(id);

		sleep(PING_DELAY);

		if (global.running == 0) {
			break;

		} else if ((count = send_to(sockfd, &data, sizeof(struct pp_data),
		                            (struct sockaddr *)&addr, len)) < 0) {
			warning("Failed to send request");
			continue;

		} else if (count == 0) {
			warning("Nothing sent");
			continue;

		} else {
			notice("Sent ping (%u)", ntohs(data.id));
		}

		memset(&data, 0, sizeof(struct pp_data));

		if ((count = recv_from(sockfd, &data, sizeof(struct pp_data),
		                       NULL, NULL)) < 0) {
			warning("Failed to receive response");

		} else if (count == 0) {
			continue;

		} else if (data.type != PONG) {
			warning("Invalid data received");

		} else if (ntohs(data.id) != id) {
			warning("Invalid id received (%u)", ntohs(data.id));

		} else {
			notice("Got  pong (%u)", ntohs(data.id));
		}

	}

	close(sockfd);
	return 0;
}

int main(int argc, char **argv)
{
	int status = EXIT_SUCCESS;
	int logopt = LOG_PID;
	int foreground = 0;
	int iopt;
	int opt;

	int talker = 0;
	char *iface = NULL;
	char *host = NULL;
	char *port = NULL;

	const struct option lopt[] = {
		{"version",	no_argument,		NULL,  0 },
		{"help",	no_argument,		NULL,  0 },
		{"interface",	required_argument,	NULL, 'i'},
		{"host",	required_argument,	NULL, 'h'},
		{"port",	required_argument,	NULL, 'p'},
		{"foreground",	no_argument,		NULL, 'f'},
		{NULL,		0,			NULL,  0 },
	};

	while ((opt = getopt_long(argc, argv, "i:h:p:f", lopt, &iopt)) != EOF) {
		switch (opt) {
		case 'i':
			if (iface == NULL && (iface = strdup(optarg)) == NULL) {
				ewarning("Failed to allocate iface");
			}

			break;

		case 'h':
			if (host == NULL) {
				if ((host = strdup(optarg)) == NULL) {
					ewarning("Failed to allocate host");

				} else {
					talker = 1;
				}
			}

			break;

		case 'p':
			if (port == NULL && (port = strdup(optarg)) == NULL) {
				ewarning("Failed to allocate port");
			}

			break;

		case 'f':
			logopt |= LOG_PERROR;
			foreground = 1;
			break;

		case 0:
			if (iopt == 0) {
				fprintf(stdout, "%s %s\n",
				        BIN_NAME, PACKAGE_VERSION);
				exit(EXIT_SUCCESS);
			}

		default:
			usage();
			exit(EXIT_FAILURE);
		}
	}

	openlog(BIN_NAME, logopt, LOG_DAEMON);

	info("Starting " BIN_NAME " in %s mode", talker ? "talker" : "listener");

	if (foreground == 0 && daemon(0, 0) < 0) {
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

	srand(time(NULL));

	if (talker == 0) {
		if (run_listener(iface ? iface : DEFAULT_IFACE,
		                 port ? port : DEFAULT_PORT) < 0) {
			critical("Failed to run listener");
			status = EXIT_FAILURE;
		}

	} else {
		if (run_talker(host, port ? port : DEFAULT_PORT) < 0) {
			critical("Failed to run talker");
			status = EXIT_FAILURE;
		}
	}

	closelog();

	free(iface);
	free(host);
	free(port);

	exit(status);
}
