CC	:= gcc
CFLAGS	:= -Wall -Wextra -Werror
CFLAGS	+= -Iinclude
LDFLAGS	:=

ifeq ("$(origin D)","command line")
CFLAGS	+= -DDEBUG
endif

CFLAGS	+= -DVERSION=\"$(shell git describe --always --dirty)\"

PHONY := all
all: daemon

daemon: daemon.o log.o network.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

PHONY += clean
clean:
	rm -f *.o

PHONY += mrproper
mrproper: clean
	rm -f daemon

.PHONY: $(PHONY)
