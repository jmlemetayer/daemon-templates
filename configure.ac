AC_INIT([net-tools],
	[m4_esyscmd_s([git describe --always --dirty])],
	[https://github.com/jmlemetayer/net-tools/issues],
	[net-tools],
	[https://github.com/jmlemetayer/net-tools])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/ping_pong.c])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_LANG([C])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h \
		  ctype.h \
		  errno.h \
		  getopt.h \
		  ifaddrs.h \
		  netdb.h \
		  signal.h \
		  syslog.h \
		  sys/socket.h \
		  sys/times.h \
		  time.h], [], AC_MSG_ERROR([header not found]))

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

# Checks for additional arguments.
AC_ARG_ENABLE([debug],
	      [AS_HELP_STRING([--enable-debug], [enable debug])],
	      [], [enable_debug=no])

AS_IF([test "x$enable_debug" != xno],
      [AC_MSG_NOTICE([debug mode is enable])
      AC_DEFINE([DEBUG], [1], [debug is enable])
      AM_SILENT_RULES([no])],
      [AM_SILENT_RULES([yes])])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
