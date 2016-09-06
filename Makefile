CC	:= gcc
CFLAGS	:= -Wall -Wextra -Werror
LDFLAGS	:=

PHONY := all
all: daemon

daemon: main.o
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
