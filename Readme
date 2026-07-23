*This project has been created as part of the 42 curriculum by oadouz, oait-si-.*

# ft_irc - Internet Relay Chat

## Description

This project is about creating our own IRC (Internet Relay Chat) server. The Internet is governed by solid standard protocols, and this project is our deep dive into understanding how real-time messaging, channels, and client-server network architectures actually work. Our server handles multiple clients simultaneously using non-blocking I/O and TCP/IP communication.

## Instructions

To get the server up and running, follow these steps:

1. **Compilation:** Use the provided `Makefile` to compile the source files. Just run:
```bash
make

```


This will compile the code using `c++` with the `-Wall -Wextra -Werror -std=c++98` flags and generate the `ircserv` executable.


2. **Execution:** Run the executable by providing a port and a password:
```bash
./ircserv <port> <password>

```


* `port`: The listening port for incoming IRC connections.


* `password`: The connection password required for any IRC client trying to connect.





---

## Under the Hood: How it Actually Works (The Low-Level Base)

Let's dive into the details and go back to the absolute base—the real low-level stuff.

First things first, we have a server and a client, right? But as developers, we live in `USER_MODE`. We have absolutely zero direct access to the hardware like the CPU, RAM, or the NIC (Network Interface Card). So, how do we build a server that clients can connect to if we can't touch the network?

That's exactly where **POSIX** comes in. POSIX is a standardization for UNIX-like systems (like Linux and macOS). It provides us with a set of rules and `syscalls` (like `socket()`, `bind()`, `poll()`). When we call them, the kernel goes and does the dirty hardware work on our behalf. We just set the rules for how our program should behave, and the kernel handles the rest.

![Scene 1: The POSIX Barrier](SVG/scene_1_posix_barrier.svg)

### The Birth of a Connection

So, how does a user communicate with our program, and how does the kernel manage it?

1. When a user connects, their kernel creates a struct with the destination IP, destination port, source IP, and source port. It sends a packet from their NIC to our server's NIC.
2. Our NIC triggers an event/interrupt to our kernel, telling it: "Hey, you got a connection!"
3. The kernel takes that packet, looks at the destination IP and port, and goes directly to a place called the **TCP Bind Hash Table**. Why? Because when we called `bind()` in our code, it placed a pointer to our server's socket struct inside this exact hash table.


4. Once it finds our socket, the kernel creates a small struct for the client called a `request_sock` (a mini struct) and places it in the **Incomplete Queue**. It stays here because the 3-way handshake isn't finished yet (this is a security layer by the kernel to protect against DDoS attacks like SYN floods).

![Scene 2: Network Interrupt and SYN Queue](SVG/scene_2_syn_queue.svg)

5. Once the final `ACK` arrives, the kernel uses a hidden internal function called `sk_clone_lock()`. It clones our listening socket, injects the important data (the 4-tuple), and upgrades it into a full `struct sock`.
6. Finally, it drops this new socket into the **Backlog Queue**. The backlog is simply a buffer managed by the operating system kernel that temporarily holds incoming network connection requests before they are explicitly accepted by our server application.

![Scene 3: Socket Creation and Accept Queue](SVG/scene_3_accept_queue.svg)



### How do we reach the Kernel from User Mode?

You might ask: *How do we access that struct in the kernel using just an `fd`?*

Here is the magic: The kernel has a **PID Hash Table**. When you want to read from something, you give it an identity (the `fd`). The kernel looks up our process's `task_struct` in the KVM (Kernel Virtual Memory). Inside `task_struct`, there is an array called `*file`.
When we call `read(fd)`, the kernel goes to our PID, checks the `*file` array at the index of our `fd` (for example, `4`), and finds the exact pointer to the socket struct the kernel created for the client!

![Scene 4: Kernel Memory Map](SVG/scene_4_memory_map.svg)

### Old Clients vs. New Clients

Does this entire process repeat every time an already connected client sends a message?
**No.**

For an old client, there is another table called the **TCP Established Hash Table**. This table holds all the active connections. When a packet arrives, the kernel checks this table first. Since it recognizes the 4-tuple (IPs and Ports), it skips the Bind table entirely.
*Correction/Note:* It does **not** put the packet in the Backlog. The Backlog is strictly for new connections. Instead, it puts the incoming data directly into the **Receive Buffer** of that specific client's socket. Then, it triggers a `POLLIN` event so we can read it with `recv()`.

![Scene 5: TCP Established Fast Path](SVG/scene_5_fast_path.svg)

### The DCC (Direct Client-to-Client) Magic

Wait, if IRC is a client-server protocol, how do users send files to each other? Do they upload a 2GB file to our server, and we send it to the other client?
**No!** That would completely choke our server's bandwidth and RAM.

Instead, IRC uses a brilliant workaround called **DCC (Direct Client-to-Client)**. Here is how it bypasses the server for heavy lifting:

1. **The Handshake (Via Server):** Client A wants to send a file to Client B. Client A sends a special `PRIVMSG` to Client B *through our server*. This message contains a CTCP (Client-To-Client Protocol).
2. **The Server's Role:** Our server doesn't know (or care) what this message means. It just acts as a router, forwarding the `PRIVMSG` to Client B exactly as it received it.
3. **The Direct Connection (Bypassing the Server):** Client B parses this CTCP message, extracts Client A's IP address and Port, and opens a **direct TCP connection** to Client A. 
4. **The Transfer:** The file is sent directly from Client A to Client B over this new socket. 

Our server is completely out of the loop and uses zero resources for the actual file transfer! This is why DCC is so efficient: the IRC server merely acts as a matchmaker to exchange IPs and ports, while the heavy data transfer is strictly Peer-to-Peer (P2P).

### The Magic of `poll()`

In our program, we call `poll()` on our file descriptors. The kernel uses the `fd` to find the main struct. Inside this struct, there is a Wait Queue (`sk_wq`).
The kernel puts our program's PID in this queue and tells our process: *"Go to sleep, rest, stop eating the CPU. I will wake you up when something happens."* So, `poll()` blocks and waits.

When the NIC receives packets and the kernel finishes routing them (either doing `sk_clone_lock` for a new client or placing data in the receive buffer for an old client), it needs to wake us up.
The kernel doesn't just write "event=POLLIN" in a variable. The event *is* the data arriving in the buffer!

Once the data is in the buffer, the kernel triggers a hidden internal callback function called `sk_data_ready`. This function looks at the Wait Queue (`sk_wq`), finds our sleeping PID, and wakes our process up. Once we are awake in `USER_MODE`, `poll()` sees that the buffer has data, returns `POLLIN`, and lets us know it's time to read.

![Scene 6: poll, Wait Queues, and Wakeup Callbacks](SVG/scene_6_poll_wait_queues.svg)

**Conclusion:**
We don't just know *what* a socket is (a communication endpoint), we know exactly *why* we use it. A socket is what gives our program access to the NIC—not directly, but through the Kernel, which handles all this massive, complex lifting for us in the background.

---

## Resources

* **RFC 1459 / RFC 2812:** The official Internet Relay Chat protocol documentation.
* **Linux Kernel Networking Documentation:** To understand the low-level behavior of TCP sockets, queues, and file descriptors.
* **AI Usage:** Artificial Intelligence was utilized to format and structure this README, translate the core technical explanations of kernel-level networking (TCP Hash Tables, Wait Queues, and `poll` mechanics) into English, and correct minor low-level inaccuracies regarding routing to the Backlog vs. the Receive Buffer, all while preserving the original authors' logical flow, architectural breakdown, and conversational style.
* **Reference Servers:** The server's parsing behavior and edge-case handling were actively tested and modeled after real-world standards by observing and strictly following the behaviors of Libera Chat and InspIRCd servers.
