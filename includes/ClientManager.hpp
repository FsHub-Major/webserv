

#pragma once

#include "Config.hpp"
#include "ext_libs.hpp"
#include "macros.hpp"

struct Client {
    int socket_fd;
    time_t last_activity;
    struct sockaddr_in address;
    bool is_active;
    
    Client() : socket_fd(0), last_activity(0), is_active(false) {}
};

class ClientManager {
private:
    Client clients[MAX_CLIENTS];
    
public:
    ClientManager();
    ~ClientManager();
    
    bool addClient(int socket_fd, const struct sockaddr_in& addr);
    void removeClient(int index);
    void removeClientBySocket(int socket_fd);
    
    void updateActivity(int socket_fd);
    void checkTimeouts();
    
    void setupSelectFds(fd_set* readfds, int* max_fd);
    void processClientActivity(const fd_set* readfds);
    
    int getClientCount() const;
    bool isFull() const;
    
private:
    int findFreeSlot();
    int findClientBySocket(int socket_fd);
    void handleClientData(int index);
    void sendHttpResponse(int socket_fd);
};
