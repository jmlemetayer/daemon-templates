# Network tools [![Build Status][badge]][travis]
The `net-tools` package include some useful packages for networking analysis and status. They are described below.

## `netpp` the network ping pong tool
This tool allow you to create a listener and a talker on two different nodes which are talking together with a datagram request and response mechanism. It is easy to setup and to check the data flow.

```
Usage: netpp [OPTIONS]...

Listener mode:
 -i, --interface IFACE   Use the specified interface (default is any).

Talker mode:
 -h, --host HOST         Use the specified host.

Common options:
 -p, --port PORT         Use the specified port (default is 1248).
 -4, --ipv4              Use IPv4 only.
 -6, --ipv6              Use IPv6 only.
 -b, --background        Daemonize.
     --version           Display version.
     --help              Display this help screen.
```

## Build and install
The `net-tools` package is using autotools:

```shell
./bootstrap # Only if you are not using a released package
./configure [--enable-debug]
make
make install
```

## License
This work is released under the [GNU General Public License v3][gplv30].

[badge]: https://travis-ci.org/jmlemetayer/net-tools.svg?branch=master
[travis]: https://travis-ci.org/jmlemetayer/net-tools
[gplv30]: https://www.gnu.org/licenses/gpl-3.0.html
