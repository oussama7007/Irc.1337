# IRC 
(Internet Relay Chat) is a text-based communication protocol used for real-time chatting over the Internet.
## Socket programming 
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




**Level 1 — Foundations (review in an hour)**

Start with what IRC actually is: a text-based protocol where clients connect to a server, authenticate, and exchange messages inside named channels. You should be able to explain the difference between a channel operator and a regular user without hesitation. Then make sure you can explain the client-server model (your `ircserv` is always the server, never initiates connections), and the basics of TCP/IP — that TCP guarantees delivery and ordering of bytes, uses a 3-way handshake, and runs over IPv4 or IPv6. Know why ports exist and what port 6667 traditionally means for IRC. Finally, be clear on what C++98 means: no lambdas, no `auto`, no range-for, no `<thread>`, no Boost — and be ready to justify any C++11 habit a peer might spot in your code.

---

**Level 2 — IRC Protocol Details (1-2 hours)**

This is often where defense questions dig deep. Every IRC message follows this format: `:prefix COMMAND param1 param2 :trailing\r\n`. The `\r\n` (CRLF) is mandatory — not just `\n`. Know by heart the authentication sequence: the client must send `PASS`, then `NICK`, then `USER` in that order before being fully registered. Know what each operator command does and its expected behavior — `KICK` with a reason, `INVITE` requiring channel membership, `TOPIC` with the `t` mode restriction, and all five `MODE` flags (`i`, `t`, `k`, `o`, `l`). Also know the key numeric replies your server sends: `001` (welcome), `353`/`366` (NAMES list), `332` (topic), `401` (no such nick), `403` (no such channel), `433` (nick already in use), `461` (not enough params), `482` (not channel operator). Evaluators will test each of these.

---

**Level 3 — Socket Programming (2-3 hours)**

This is the core of the project. Know what a file descriptor is — a small integer the kernel gives you to refer to an open resource (sockets are file descriptors). Walk through the server setup sequence in order: `socket()` creates the socket, `setsockopt()` with `SO_REUSEADDR` avoids the "address already in use" error on restart, `bind()` attaches it to your port, `listen()` marks it as passive, and `accept()` blocks until a client connects (or returns immediately in non-blocking mode). For data transfer, know that `send()` and `recv()` may not send/receive all bytes in one call — you must loop. Know `getaddrinfo()` / `freeaddrinfo()` for resolving addresses, and understand byte order: the network uses big-endian, your machine is likely little-endian, so `htons()` converts a port from host to network byte order before `bind()`. Be able to explain `inet_ntop()` vs `inet_ntoa()` (the latter is deprecated and not thread-safe).

---

**Level 4 — Non-blocking I/O (2-3 hours)**

A blocking socket call (`accept`, `recv`, `send`) puts your process to sleep until data is available. With multiple clients, that means one slow client can freeze everyone else — which is why the subject forbids blocking I/O. You set a file descriptor to non-blocking with `fcntl(fd, F_SETFL, O_NONBLOCK)` — this is the **only** `fcntl` usage allowed. After that, `recv()` on a socket with no data doesn't block; it returns `-1` and sets `errno` to `EAGAIN` or `EWOULDBLOCK`. This is not an error — it means "nothing to read right now, try later." Be ready to explain this clearly because it's a classic defense question. Similarly, `accept()` on a non-blocking socket returns `-1`/`EAGAIN` when no pending connections exist. You must handle these cases without crashing or exiting.

---

**Level 5 — `poll()` and the Event Loop (3-4 hours, the hardest to explain clearly)**

This is where everything comes together. `poll()` takes an array of `struct pollfd` (each has a `fd`, an `events` field you set, and a `revents` field the kernel fills in) and a timeout. It blocks until at least one fd is ready, then returns. `POLLIN` means "data available to read or a new connection on the listening socket." `POLLOUT` means "safe to write without blocking." The rule that only **one** `poll()` can be used means your entire server runs in a single loop: call `poll()`, iterate over all fds, handle the ready ones, repeat. You must be able to draw this loop on a whiteboard. Know why `select()` has a hard limit (`FD_SETSIZE` = 1024) while `poll()` doesn't. Know that `epoll` (Linux) and `kqueue` (macOS/BSD) are more efficient for huge numbers of fds but are overkill here. The key point the subject makes: if you call `read()`/`recv()` on a fd that wasn't flagged ready by `poll()`, your grade is 0 — even if the call happens to succeed.

---

**Level 6 — Advanced / Defense Critical (ongoing, hardest to internalize)**

**Partial data / buffering** is the subtlest bug source. TCP is a stream protocol — there is no guarantee that a full IRC message arrives in one `recv()` call. Your server must accumulate bytes in a per-client buffer and only process a command once a full `\r\n`-terminated message is assembled. The `nc` test in the subject (sending `com`, then `man`, then `d\n` with Ctrl+D) directly tests this. Make sure you can trace through what your buffer code does in that scenario.

**Signal handling**: `SIGPIPE` is the one that kills most students. When you `send()` to a client that has disconnected, the kernel sends your process `SIGPIPE`, which terminates it by default. You must either ignore it (`signal(SIGPIPE, SIG_IGN)`) or handle it with `sigaction()`. Know `sigemptyset`, `sigfillset`, `sigaddset` — the evaluator may ask what they do.

**Write buffering**: if a client's send buffer is full, `send()` returns fewer bytes than requested (or `-1`/`EAGAIN`). You need an output queue per client and watch for `POLLOUT` to drain it — otherwise you silently drop data.

**Memory safety**: no leaks, no double-frees, no use-after-free. Be able to explain what happens when a client disconnects — you remove their fd from the `pollfd` array, free/destroy their state, and close the fd. Order matters.

The one question that sums up Level 6: *"Walk me through exactly what happens from the moment a client types a message and hits Enter until every other client in the channel receives it."* If you can answer that end-to-end without gaps, you're ready.



=========================
achno khass mli kadir nc -C ip ...and join chi channel direct bla matregesterach khass ytele3 lik message had sat khass ydir register, project mli kaydir join #channel name kaydekhelo direct 



ila kenty deja member f channel o derti 3awtani join l nafss channel makhasch ydir lik join , khass maytle3 walo lik o lnass li f same channel 

=========================
your serv 
privmsg #ll "hello world"
:server 401 bb #ll :No such nick/channel


liberaserv 
privmsg #ll "hello world"
:rr!~rr@197.230.30.146 PRIVMSG #ll :"hello

=========================
right irc 
mode #yy +l abc
mode #yy +l 1
:yynick!~yy@197.230.30.146 MODE #yy +l 1 


your serv 
mode +test +l abc
:bb!bb@localhost MODE +test +l abc
:bb!bb@localhost MODE +test +l abc
should not print those two lines cus abc is false l require num  even if it like 1a but not a1 

ila kant mode #channel +l 0 ola +l abc khassk matele3ch chi message 
ils kant mode #channel +l khaasek tele3  :Not enough parameters


=========================