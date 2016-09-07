#include "log.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

#define DUMPSIZE		16

#ifdef DEBUG
void dump(const char *prefix, const void *buf, size_t count)
{
	char hex[2 * DUMPSIZE + 1];
	char str[DUMPSIZE + 1];
	size_t i;
	char *h;
	char *s;

	for (i = 0; i < count;) {
		for (h = hex, s = str;
		     (h + 1 < hex + sizeof(hex)) && (i < count);
		     h += 2, s++, i++) {
			snprintf(h, 3, "%.2X", ((uint8_t *)buf)[i]);

			if (isprint((int)((uint8_t *)buf)[i]) == 0) {
				snprintf(s, 2, ".");

			} else {
				snprintf(s, 2, "%c", ((uint8_t *)buf)[i]);
			}
		}

		debug("%6s: %-*s |%s|", prefix, 2 * DUMPSIZE, hex, str);
	}
}
#endif
