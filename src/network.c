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
		info("Listening on %s:%s", addr ? addr : "*", port);
		return sockfd;
	}

	freeaddrinfo(infos);
	error("Failed to get socket");
	return -1;
}

int get_listener(const char *iface, const char *port)
{
	struct ifaddrs *infos;
	struct ifaddrs *info;
	char host[NI_MAXHOST];
	socklen_t len;
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
			len = sizeof(struct sockaddr_in);

		} else if (info->ifa_addr->sa_family == AF_INET6) {
			len = sizeof(struct sockaddr_in6);

		} else {
			continue;
		}

		if ((err = getnameinfo(info->ifa_addr, len, host, NI_MAXHOST,
		                       NULL, 0, NI_NUMERICHOST)) < 0) {
			error("Failed to get interface name "
			      "info: %s", gai_strerror(err));
			continue;

		} else if ((sockfd = get_listener_addr(host, port)) < 0) {
			notice("Failed to get listener from "
			       "host address %s", host);
			continue;
		}

		freeifaddrs(infos);
		return sockfd;
	}

	freeifaddrs(infos);
	error("Failed to get interface address");
	return -1;
}

int get_talker(const char *host, const char *port,
               struct sockaddr *addr, socklen_t *len)
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
		}

		memcpy(addr, info->ai_addr, sizeof(struct sockaddr));
		*len = info->ai_addrlen;

		freeaddrinfo(infos);
		info("Talking to %s:%s", host, port);
		return sockfd;
	}

	freeaddrinfo(infos);
	error("Failed to get socket");
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

ssize_t recv_from(int sockfd, void *buf, size_t count,
                  struct sockaddr *addr, socklen_t *len)
{
	ssize_t recv;

	if ((recv = wait_data(sockfd, DATA_TIMEOUT_SEC)) < 0) {
		error("Failed to wait for data");
		return -1;

	} else if (recv == 0) {
		return 0;

	} else if ((recv = recvfrom(sockfd, buf, count, 0, addr, len)) < 0) {
		eerror("Failed to receive data");
		return -1;
	}

	dump("recv", buf, recv);

	return recv;
}

ssize_t send_to(int sockfd, const void *buf, size_t count,
                const struct sockaddr *addr, socklen_t len)
{
	ssize_t send;

	dump("send", buf, count);

	if ((send = sendto(sockfd, buf, count, 0, addr, len)) < 0) {
		eerror("Failed to send data");
		return -1;
	}

	return send;
}
