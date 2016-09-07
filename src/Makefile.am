AM_CPPFLAGS  = -Wall -Wextra -Werror
AM_CPPFLAGS += -I$(srcdir)/../include

bin_PROGRAMS = netpp

netpp_SOURCES  = ping_pong.c
netpp_SOURCES += log.c ../include/log.h
netpp_SOURCES += network.c ../include/network.h
netpp_CPPFLAGS = $(AM_CPPFLAGS) -DBIN_NAME=\"netpp\"
