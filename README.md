# webserv
HTTP server from scratch 


## Multiplexing 
Multiplexing comes from the concept of "multiple signals over a single channel" - the term originates from telecommunications and electronics.


### Multiplexing logic uses 'select' : 

#### key features: 

- No thread context switching (single-threaded)
- Non-blocking I/O (after `select()` returns)
- Kernel-based event notification (relies on OS efficiency)
- Process sleeps until activity (CPU efficient)


1. select() BLOCKS ────► Kernel monitors: server_fd, client_sockets[0-19]
                                                    │
2. Network activity ───► fd5 receives data ────────┘
                                                    │
3. Kernel wakes up ────► select() RETURNS ◄────────┘
                                                    │
4. Your code checks ───► FD_ISSET(fd5, &readfds) = true
                                                    │
5. Handle data ────────► read(fd5, buffer, ...)   ◄┘
                                                    │
6. Loop back ──────────► select() BLOCKS again ◄──┘

