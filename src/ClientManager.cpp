#include "ClientManager.hpp"
#include "webserv.hpp"
#include "macros.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

// to delete, maybe not 
#include <fcntl.h>

ClientManager::ClientManager() {

    std::cout << " ClientManager Constructor called" << std::endl;
}

ClientManager::~ClientManager() {
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it) {
        close(it->first); 
    }
    clients.clear();
    std::cout << " ~ClientManager called" << std::endl;
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

// poll variant: readable_fds are those with POLLIN ready
void ClientManager::processClientRequestPoll(const std::vector<int>& readable_fds) {
    for (size_t i = 0; i < readable_fds.size(); ++i) {
        int socket_fd = readable_fds[i];
        std::map<int, Client>::iterator it = clients.find(socket_fd);
        if (it == clients.end())
            continue; // might have been removed already
        char buffer[1024] = {0};
        int valread = read(socket_fd, buffer, sizeof(buffer));
        if (valread == 0) {
            // client closed
            removeClient(socket_fd);
            continue;
        } else if (valread > 0) {
            buffer[valread] = '\0';
            sendHttpResponse(socket_fd);
            updateActivity(socket_fd);
        } else {
            perror("read");
            removeClient(socket_fd);
        }
    }
}

void ClientManager::sendHttpResponse(int socket_fd) {
    std::string http_response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello World!";
    send(socket_fd, http_response.c_str(), http_response.length(), 0);
}

void ClientManager::updateActivity(int socket_fd) {
    std::map<int, Client>::iterator it = clients.find(socket_fd);
    if (it != clients.end()) {
        it->second.last_activity = time(NULL);
    }
}


void ClientManager::checkTimeouts()
{
    time_t current_time = time(NULL);
    std::map<int, Client>::iterator it = clients.begin();
    while (it != clients.end())
    {
        if (current_time - it->second.last_activity > CLIENT_TIMEOUT)
        {
            printf("Client timeout: socket fd %d\n", it->first);
            int socket_fd = it->first;
            ++it;
            removeClient(socket_fd);
        }
        else{
            ++it;
        }

    }
}

