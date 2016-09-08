#ifndef _LOG_H_
#define _LOG_H_

#include <errno.h>
#include <string.h>
#include <syslog.h>

#ifdef DEBUG
#define debug(format, ...)	syslog(LOG_DEBUG, "D/ " format, ##__VA_ARGS__)
void dump(const char *prefix, const void *buf, size_t len);
#else
#define debug(format, ...)
#define dump(prefix, buf, len);
#endif

#define info(format, ...)	syslog(LOG_INFO, "I/ " format, ##__VA_ARGS__)
#define notice(format, ...)	syslog(LOG_NOTICE, "N/ " format, ##__VA_ARGS__)
#define warning(format, ...)	syslog(LOG_WARNING, "W/ " format, ##__VA_ARGS__)
#define error(format, ...)	syslog(LOG_ERR, "E/ " format, ##__VA_ARGS__)
#define critical(format, ...)	syslog(LOG_CRIT, "C/ " format, ##__VA_ARGS__)

#define ewarning(format, ...)	warning(format ": %s", ##__VA_ARGS__, strerror(errno))
#define eerror(format, ...)	error(format ": %s", ##__VA_ARGS__, strerror(errno))
#define ecritical(format, ...)	critical(format ": %s", ##__VA_ARGS__, strerror(errno))

#endif
