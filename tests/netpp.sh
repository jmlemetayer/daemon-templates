#!/bin/sh
NETPP=./netpp
TMPFILE=$(mktemp)
fail() { echo >&2 FAIL: $@; exit 99; }
trap "killall -q -y5s netpp; rm -f $TMPFILE" EXIT
# Check program
[ -x $NETPP ] || fail no netpp program
# Start listener in background mode
$NETPP --background || fail listener not started
# Start talker and store logs in a temprorary file
$NETPP --host localhost 2>&1 | tee $TMPFILE &
# Wait a little and kill every netpp
sleep 2 && killall -y5s netpp || fail no netpp killed
# Check the temporary file
[ -n "$(cat $TMPFILE | grep acknowledged)" ] && exit 0 || fail no acknowledge
