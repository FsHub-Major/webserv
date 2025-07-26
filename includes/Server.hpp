


#pragma once

#include "Config.hpp"
#include <socket.hpp>


class Server {

    private:
        const ServerConfig config;
        ClientManager clients; 

        bool is_running;
        bool is_init;

    public:
        Server(const ServerConfig &config);
        ~Server();

        bool init();
        void clean();

        void run();
        void stop();

        bool handleConnection();
        void processRequest(fd_set * readfds);
    
    private:




}