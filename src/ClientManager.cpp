

#include "ClientManager.hpp"
#include "webserv.hpp"
#include "macros.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

ClientManager::ClientManager() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = Client();
    }
}

ClientManager::~ClientManager() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].is_active) {
            close(clients[i].socket_fd);
        }
    }
}


bool ClientManager::addClient(int socket_fd, const struct sockaddr_in& addr) {
    if (isFull()) {
        std::cerr << "ClientManager: Maximum clients reached (" << MAX_CLIENTS << ")" << std::endl;
        return false;
    }
    
    Client new_client;
    new_client.socket_fd = socket_fd;
    new_client.address = addr;
    new_client.last_activity = time(NULL);
    new_client.is_active = true;
    
    clients[socket_fd] = new_client;
    
    printf("Client added: socket fd %d, IP %s, port %d (total: %d)\n",
           socket_fd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), 
           getClientCount());
    
    return true;
}


void ClientManager::removeClient(int socket_fd) {
    std::map<int, Client>::iterator it = clients.find(socket_fd);
    if (it != clients.end()) {
        printf("Client disconnected: socket fd %d, IP %s, port %d (remaining: %d)\n",
               it->first,
               inet_ntoa(it->second.address.sin_addr),
               ntohs(it->second.address.sin_port),
               getClientCount() - 1);
        
        close(it->first);
        clients.erase(it);
    }
}





int ClientManager::getClientCount() const {
    return (clients.size()); // cast to int ?  
}

bool ClientManager::isFull() const {
    return clients.size() >= MAX_CLIENTS;
}



void ClientManager::updateActivity(int socket_fd) {
    std::map<int, Client>::iterator it = clients.find(socket_fd);
    if (it != clients.end()) {
        it->second.last_activity = time(NULL);
    }
}
