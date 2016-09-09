AM_CPPFLAGS  = -Wall -Wextra -Werror
AM_CPPFLAGS += -I$(srcdir)/include

bin_PROGRAMS = netpp

netpp_SOURCES  = src/ping_pong.c
netpp_SOURCES += src/log.c include/log.h
netpp_SOURCES += src/network.c include/network.h
netpp_CPPFLAGS = $(AM_CPPFLAGS) -DBIN_NAME=\"netpp\"
