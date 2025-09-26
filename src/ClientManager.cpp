#include "ClientManager.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "webserv.hpp"
#include "macros.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

// to delete, maybe not 
#include <fcntl.h>

ClientManager::ClientManager(const ServerConfig & config) : config(config) {

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

std::string ClientManager::readFullRequest(int socket_fd) {
    std::string request;
    char buffer[BUFF_SIZE];
    
    while (true) {
        int valread = read(socket_fd, buffer, sizeof(buffer));
        if (valread <= 0) {
            return "";  // Connection closed or error
        }
        
        request.append(buffer, valread);
        
        // Check if headers are complete (double CRLF)
        if (request.find("\r\n\r\n") != std::string::npos) {
            // Check if body is complete (if Content-Length present)
            size_t header_end = request.find("\r\n\r\n");
            std::string headers = request.substr(0, header_end);
            
            // Extract Content-Length (use stringstream instead of atoi)
            size_t cl_pos = headers.find("Content-Length: ");
            if (cl_pos != std::string::npos) {
                size_t cl_end = headers.find("\r\n", cl_pos);
                std::string cl_str = (cl_end == std::string::npos)
                    ? headers.substr(cl_pos + 16)
                    : headers.substr(cl_pos + 16, cl_end - cl_pos - 16);
                size_t content_length = 0;
                {
                    std::istringstream iss(cl_str);
                    // On failure, content_length remains 0 (mirrors atoi behavior)
                    iss >> content_length;
                    if (content_length < 0) content_length = 0;
                }

                // Calculate body size
                size_t body_start = header_end + 4;
                size_t current_body_size = request.size() - body_start;

                if (current_body_size < static_cast<size_t>(content_length)) {
                    continue; 
                }
            }
            
            return request;
        }
    }
}

// poll variant: readable_fds are those with POLLIN ready
void ClientManager::processClientRequestPoll(const std::vector<int>& readable_fds) {
    for (size_t i = 0; i < readable_fds.size(); ++i) {
        int socket_fd = readable_fds[i];


        std::string raw_request = readFullRequest(socket_fd);

        if (raw_request.empty())
        {
            removeClient(socket_fd);
            continue;
        }

        
        std::cout << "RAW REQUEST" << std::endl;
        std::cout << raw_request << std::endl;
        HttpRequest request;
        std::string response;
        if (request.parseRequest(raw_request, this->config.root))
            response = HttpResponse::createResponse(request, this->config);
        else
            response = "HTTP/1.0 400 Bad Request\r\n\r\n";

        ssize_t sent = send(socket_fd , response.c_str(), response.length(), 0);
        if (sent == -1)
        {
            std::cerr << "Send failed for socket " << socket_fd << std::endl;
            removeClient(socket_fd);
            continue;
        }
        updateActivity(socket_fd);
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

