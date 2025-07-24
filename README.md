# webserv
HTTP server from scratch 



Multiplexing logic uses 'select' : 

No context switching between threads / non-blocking I/O / relies on kernel for event notification / process sleeps until kernel signals activity

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

