#include "network.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"

#define DATA_TIMEOUT_SEC	2

#define logsaddr(level, prefix, sa, salen) { \
	char host[NI_MAXHOST]; \
	char serv[NI_MAXSERV]; \
	if (getnameinfo(sa, salen, \
			host, sizeof(host), serv, sizeof(serv), \
			NI_NUMERICHOST | NI_NUMERICSERV) == 0) { \
		level(prefix " %s:%s", host, serv); \
	} \
}

static int get_listener_addr(const char *addr, const char *port)
{
	struct addrinfo *infos;
	struct addrinfo *info;
	struct addrinfo hints;
	int sockfd;
	int err;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if (addr == NULL) {
		hints.ai_flags = AI_PASSIVE;
	}

	if ((err = getaddrinfo(addr, port, &hints, &infos)) != 0) {
		error("Failed to get address info: %s", gai_strerror(err));
		return -1;
	}

	for (info = infos; info != NULL; info = info->ai_next) {
		if ((sockfd = socket(info->ai_family, info->ai_socktype,
		                     info->ai_protocol)) < 0) {
			eerror("Failed to create socket");
			continue;

		} else if (bind(sockfd, info->ai_addr, info->ai_addrlen) < 0) {
			eerror("Failed to bind socket");
			close(sockfd);
			continue;
		}

		freeaddrinfo(infos);
		logsaddr(info, "Listening on", info->ai_addr, info->ai_addrlen);
		return sockfd;
	}

	freeaddrinfo(infos);
	error("Failed to get listener socket for address: "
	      "%s:%s", addr ? addr : "*", port);
	return -1;
}

int get_listener(const char *iface, const char *port)
{
	struct ifaddrs *infos;
	struct ifaddrs *info;
	char host[NI_MAXHOST];
	socklen_t salen;
	int sockfd;
	int err;

	debug("Requesting listener socket on %s:%s", iface, port);

	if (strcmp(iface, WILDCARD_IFACE) == 0) {
		return get_listener_addr(NULL, port);

	} else if (getifaddrs(&infos) < 0) {
		eerror("Failed to get interface info");
		return -1;
	}

	for (info = infos; info != NULL; info = info->ifa_next) {
		if (strcmp(iface, info->ifa_name) != 0) {
			continue;

		} else if (info->ifa_addr == NULL) {
			continue;

		} else if (info->ifa_addr->sa_family == AF_INET) {
			salen = sizeof(struct sockaddr_in);

		} else if (info->ifa_addr->sa_family == AF_INET6) {
			salen = sizeof(struct sockaddr_in6);

		} else {
			continue;
		}

		if ((err = getnameinfo(info->ifa_addr, salen,
		                       host, sizeof(host),
		                       NULL, 0, NI_NUMERICHOST)) < 0) {
			error("Failed to get interface name "
			      "info: %s", gai_strerror(err));
			continue;

		} else if ((sockfd = get_listener_addr(host, port)) < 0) {
			notice("Failed to get listener socket from "
			       "host address %s", host);
			continue;
		}

		freeifaddrs(infos);
		return sockfd;
	}

	freeifaddrs(infos);
	error("Failed to get listener socket on: %s:%s", iface, port);
	return -1;
}

int get_talker(const char *host, const char *port,
               struct sockaddr *sa, socklen_t *salen)
{
	struct addrinfo *infos;
	struct addrinfo *info;
	struct addrinfo hints;
	int sockfd;
	int err;

	debug("Requesting talker socket on %s:%s", host, port);

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ((err = getaddrinfo(host, port, &hints, &infos)) != 0) {
		error("Failed to get address info: %s", gai_strerror(err));
		return -1;
	}

	for (info = infos; info != NULL; info = info->ai_next) {
		if ((sockfd = socket(info->ai_family, info->ai_socktype,
		                     info->ai_protocol)) < 0) {
			eerror("Failed to create socket");
			continue;

		} else if (*salen > info->ai_addrlen) {
			*salen = info->ai_addrlen;
		}

		memcpy(sa, info->ai_addr, *salen);

		freeaddrinfo(infos);
		logsaddr(info, "Talking to", info->ai_addr, info->ai_addrlen);
		return sockfd;
	}

	freeaddrinfo(infos);
	error("Failed to get talker socket on: %s:%s", host, port);
	return -1;
}

static int wait_data(int sockfd, ssize_t timeout)
{
	struct timeval time;
	fd_set set;

	FD_ZERO(&set);
	FD_SET(sockfd, &set);

	time.tv_sec = timeout;
	time.tv_usec = 0;

	if (select(sockfd + 1, &set, NULL, NULL, &time) < 0) {
		if (errno == EINTR) {
			return 0;
		}

		eerror("Failed to wait on socket");
		return -1;

	} else if (FD_ISSET(sockfd, &set)) {
		return 1;

	} else {
		return 0;
	}
}

ssize_t recv_from(int sockfd, void *buf, size_t len,
                  struct sockaddr *sa, socklen_t *salen)
{
	ssize_t recv;
	int ret;

	struct sockaddr_storage rsa;
	socklen_t rsalen = sizeof(rsa);
	struct sockaddr *psa = sa ? sa : (struct sockaddr *)&rsa;
	socklen_t *psalen = salen ? salen : &rsalen;

	if ((ret = wait_data(sockfd, DATA_TIMEOUT_SEC)) < 0) {
		error("Failed to wait for data");
		return -1;

	} else if (ret == 0) {
		return 0;

	} else if ((recv = recvfrom(sockfd, buf, len, 0, psa, psalen)) < 0) {
		eerror("Failed to receive data");
		return -1;
	}

	logsaddr(debug, "Received from", psa, *psalen);
	dump("recv", buf, recv);

	return recv;
}

ssize_t send_to(int sockfd, const void *buf, size_t len,
                const struct sockaddr *sa, socklen_t salen)
{
	ssize_t send;

	logsaddr(debug, "Sending to   ", sa, salen);
	dump("send", buf, len);

	if ((send = sendto(sockfd, buf, len, 0, sa, salen)) < 0) {
		eerror("Failed to send data");
		return -1;
	}

	return send;
}
