


#pragma once

#include "Config.hpp"
#include "ClientManager.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#ifdef __linux__
#include <sys/epoll.h>
#endif
#include <vector>
#include <iostream>

class Server {

    private:
        const ServerConfig config;
        ClientManager clients;
        sockaddr_in address ;

        int server_fd;
    bool is_running;
    bool is_init;
    // poll(2) related container rebuilt each loop for clarity
    std::vector< struct pollfd > poll_fds;

#ifdef __linux__
    int epoll_fd; // epoll instance on Linux
#endif

    public:
        Server(const ServerConfig &config);
        ~Server();


        bool init();
        void run();
        void stop();


    // poll-based helpers
    bool handleNewConnection();
    void buildPollFds();
    void processReadyFds();

        int getPort() const;
        int getServerFd() const;
        bool isRunning() const;
        bool isInitialized() const;


    private:
        Server(const Server&);
        Server& operator=(const Server&);
        void cleanup();
        
};