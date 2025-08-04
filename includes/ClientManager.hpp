

#pragma once

#include "Config.hpp"
#include "ext_libs.hpp"
#include "macros.hpp"
#include <map>

struct Client {
    int socket_fd;
    time_t last_activity;
    struct sockaddr_in address;
    bool is_active;
    
    Client() : socket_fd(0), last_activity(0), is_active(false) {}
};

class ClientManager {
private:
    std::map<int, Client> clients;
    
public:
    ClientManager();
    ~ClientManager();
    
    bool addClient(int socket_fd, const struct sockaddr_in& addr);
    void removeClient(int socket_fd, fd_set * readfds);
    
    void updateActivity(int socket_fd);
    void checkTimeouts(fd_set *readfds);
    
    void setupClientFds(fd_set* readfds, int* max_fd);
    void processClientRequest(fd_set* readfds);
    void cleanupInvalidFds(fd_set * readfds); // to delete, maybe 
    
    int getClientCount() const;
    bool isFull() const;
    
private:
    int findFreeSlot();
    int findClientBySocket(int socket_fd);
    void handleClientData(int index);
    void sendHttpResponse(int socket_fd);
};
