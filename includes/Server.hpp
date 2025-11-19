
#pragma once

#include "ClientManager.hpp"
#include "Config.hpp"

#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#ifdef __linux__
#include <sys/epoll.h>
#endif
#include <vector>

class Server {
public:
    explicit Server(const ServerConfig &config);
    ~Server();

    bool init();
    void run();
    void stop();

    int getPort() const;
    int getServerFd() const;
    bool isRunning() const;
    bool isInitialized() const;

private:
    Server(const Server&);
    Server& operator=(const Server&);

    bool handleNewConnection();
    void buildPollFds();
    void processReadyFds();
    void cleanup();

    const ServerConfig config;
    ClientManager clients;
    sockaddr_in address;
    int server_fd;
    bool is_running;
    bool is_init;
    std::vector<struct pollfd> poll_fds; // rebuilt each loop for clarity

#ifdef __linux__
    int epoll_fd; // epoll instance on Linux
#endif
};