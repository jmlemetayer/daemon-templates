#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <sys/socket.h>
#include <sys/types.h>

#define WILDCARD_IFACE  "any"

#define DEFAULT_IFACE   WILDCARD_IFACE
#define DEFAULT_PORT    "1248"

int get_listener(const char *iface, const char *port);
int get_talker(const char *host, const char *port,
               struct sockaddr *sa, socklen_t *salen);

ssize_t recv_from(int sockfd, void *buf, size_t len,
                  struct sockaddr *sa, socklen_t *salen);
ssize_t send_to(int sockfd, const void *buf, size_t len,
                const struct sockaddr *sa, socklen_t salen);

#endif
