#include <arpa/inet.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
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
	        " -4, --ipv4              Use IPv4 only.\n"
	        " -6, --ipv6              Use IPv6 only.\n"
	        " -b, --background        Daemonize.\n"
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

#define PING 1
#define PONG 2

struct __attribute__((packed)) pp_data {
	uint8_t type;
	uint16_t id;
	union {
		struct timeval start;
		uint8_t pad[16];
	} payload;
#define start payload.start
};

static int run_listener(const char *iface, const char *port, int family)
{
	struct sockaddr_storage sa;
	struct pp_data data;
	socklen_t salen;
	ssize_t count;
	int sockfd;

	if ((sockfd = get_listener(iface, port, family)) < 0) {
		error("Failed to get listener socket");
		return -1;
	}

	while (global.running == 1) {
		memset(&data, 0, sizeof(data));
		salen = sizeof(sa);

		if ((count = recv_from(sockfd, &data, sizeof(data),
		                       (struct sockaddr *)&sa, &salen)) < 0) {
			warning("Failed to receive request");
			continue;

		} else if (count == 0) {
			continue;

		} else if (data.type != PING) {
			warning("Invalid data received");
			continue;
		}

		data.type = PONG;

		if ((count = send_to(sockfd, &data, sizeof(data),
		                     (struct sockaddr *)&sa, salen)) < 0) {
			warning("Failed to send response");

		} else if (count == 0) {
			warning("Nothing sent");

		} else {
			notice("Sequence acknowledged (%u)", ntohs(data.id));
		}
	}

	close(sockfd);
	return 0;
}

static double diff_ms(struct timeval *time)
{
	struct timeval cur;

	gettimeofday(&cur, NULL);

	return ((((double)cur.tv_sec - (double)time->tv_sec) * 1000000.0) +
	        ((double)cur.tv_usec - (double)time->tv_usec)) / 1000.0;
}

static int run_talker(const char *host, const char *port, int family)
{
	struct sockaddr_storage sa;
	socklen_t salen = sizeof(sa);
	uint16_t id = rand();
	struct pp_data data;
	ssize_t count;
	int sockfd;

	if ((sockfd = get_talker(host, port, family,
	                         (struct sockaddr *)&sa, &salen)) < 0) {
		error("Failed to get talker socket");
		return -1;
	}

	while (global.running == 1) {
		sleep(PING_DELAY);

		id += 1;
		data.type = PING;
		data.id = htons(id);
		gettimeofday(&data.start, NULL);

		if (global.running == 0) {
			break;
		}

		notice("Sending ping pong sequence        (%u)", id);

		if ((count = send_to(sockfd, &data, sizeof(data),
		                     (struct sockaddr *)&sa, salen)) < 0) {
			warning("Failed to send request");
			continue;

		} else if (count == 0) {
			warning("Nothing sent");
			continue;
		}

		memset(&data, 0, sizeof(data));

		if ((count = recv_from(sockfd, &data, sizeof(data),
		                       NULL, NULL)) < 0) {
			warning("Failed to receive response");

		} else if (count == 0) {
			continue;

		} else if (data.type != PONG) {
			warning("Invalid data received");

		} else if (ntohs(data.id) != id) {
			warning("Invalid id received (%u)", ntohs(data.id));

		} else {
			notice("Sequence acknowledged in %.2f ms (%u)",
			       diff_ms(&data.start), ntohs(data.id));
		}

	}

	close(sockfd);
	return 0;
}

int main(int argc, char **argv)
{
	int logopt = LOG_PID | LOG_PERROR;
	int status = EXIT_SUCCESS;
	int family = AF_UNSPEC;
	int background = 0;
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
		{"background",	no_argument,		NULL, 'b'},
		{"ipv4",	no_argument,		NULL, '4'},
		{"ipv6",	no_argument,		NULL, '6'},
		{NULL,		0,			NULL,  0 },
	};

	while ((opt = getopt_long(argc, argv, "i:h:p:b46", lopt, &iopt)) != EOF) {
		switch (opt) {
		case 'i':
			if (iface == NULL && (iface = strdup(optarg)) == NULL) {
				fprintf(stderr,
				        "W/ Failed to allocate iface: %s",
				        strerror(errno));
			}

			break;

		case 'h':
			if (host == NULL) {
				if ((host = strdup(optarg)) == NULL) {
					fprintf(stderr,
					        "W/ Failed to allocate host: %s",
					        strerror(errno));

				} else {
					talker = 1;
				}
			}

			break;

		case 'p':
			if (port == NULL && (port = strdup(optarg)) == NULL) {
				fprintf(stderr,
				        "W/ Failed to allocate port: %s",
				        strerror(errno));
			}

			break;

		case 'b':
			logopt &= ~LOG_PERROR;
			background = 1;
			break;

		case '4':
			if (family == AF_UNSPEC) {
				family = AF_INET;
			}

			break;

		case '6':
			if (family == AF_UNSPEC) {
				family = AF_INET6;
			}

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

	if (background != 0 && daemon(0, 0) < 0) {
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
		                 port ? port : DEFAULT_PORT,
		                 family) < 0) {
			critical("Failed to run listener");
			status = EXIT_FAILURE;
		}

	} else {
		if (run_talker(host, port ? port : DEFAULT_PORT, family) < 0) {
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
