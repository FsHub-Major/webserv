


#pragma once

#include "Config.hpp"
#include "ClientManager.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>

class Server {

    private:
        const ServerConfig config;
        ClientManager clients;
        sockaddr_in address ;

        int port;
        int server_fd;
        bool is_running;
        bool is_init;

    public:
        Server(const ServerConfig &config);
        ~Server();


        bool init();
        void run();
        void stop();

        bool handleNewConnection();
        void processRequest(fd_set * readfds);

        int getPort() const;
        int getServerFd() const;
        bool isRunning() const;
        bool isInitialized() const;


    private:
        Server(const Server&);
        Server& operator=(const Server&);
        void cleanup();
        
};