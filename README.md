

# Socket programming 
in C++ allows programs to communicate over a network by sending and receiving data packets

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
Its role is to create a socket endpoint (a special file descriptor used for network communication).

the parameters are:

#### 1 AF_INET → Address family
IPv4
Use AF_INET6 for IPv6.
#### SOCK_STREAM → Socket type
TCP (connection-oriented, reliable stream).
SOCK_DGRAM would be UDP.
#### 0 → Protocol
0 means "use the default protocol for this address family and socket type."
Since AF_INET + SOCK_STREAM corresponds to TCP, the kernel automatically chooses TCP.
Since AF_INET + SOCK_DGRAM corresponds to UDP, the kernel automatically chooses UDP.


