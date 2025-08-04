


#pragma once

#include <sys/select.h>
#include <cstdio>

class Debug 
{
    public:
        static inline void inspectReadfds(fd_set* readfds, int max_fd, int server_fd) {
        printf("=== Active file descriptors in readfds ===\n");
    
        // Check server socket
        if (FD_ISSET(server_fd, readfds)) {
            printf("Server socket %d is ready\n", server_fd);
        }
    
        // Check all possible client sockets
        for (int fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, readfds)) {
                if (fd == server_fd) {
                    printf("FD %d: Server socket (new connection pending)\n", fd);
                } else {
                    printf("FD %d: Client socket (data ready)\n", fd);
                }
            }
        }
        printf("=== End of readfds inspection ===\n");
    }

    static inline int countActiveFds(fd_set* readfds, int max_fd) {
        int count = 0;
        for (int fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, readfds)) {
                count++;
            }
        }
        return count;
    }


};

