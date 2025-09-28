#include "ClientManager.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "webserv.hpp"
#include "macros.hpp"
#include "ext_libs.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

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
    
    // Initialize new client with current timestamp
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


// Simple getter methods
int ClientManager::getClientCount() const {
    return static_cast<int>(clients.size());
}

bool ClientManager::isFull() const {
    return clients.size() >= MAX_CLIENTS;
}

std::string ClientManager::readFullRequest(int socket_fd) {
    std::string request;
    char buffer[BUFF_SIZE];
    
    for (;;) {
        int bytes_read = read(socket_fd, buffer, sizeof(buffer));
        if (bytes_read <= 0) return ""; // Connection closed or error

        request.append(buffer, bytes_read);

        // Look for end of HTTP headers
        size_t header_end = request.find("\r\n\r\n");
        if (header_end == std::string::npos) continue; // Keep reading headers

        // Check if request has body with Content-Length
        const std::string headers = request.substr(0, header_end);
        size_t cl_pos = headers.find("Content-Length: ");
        if (cl_pos != std::string::npos) {
            // Extract and parse Content-Length value
            size_t cl_end = headers.find("\r\n", cl_pos);
            std::string cl_val = headers.substr(cl_pos + 16, cl_end - cl_pos - 16);
            cl_val = trim(cl_val, " \t\r\n");
            int content_length = stringtoi(cl_val);
            if (content_length < 0) content_length = 0;

            // Ensure we have the complete body
            size_t body_start = header_end + 4;
            size_t body_received = request.size() - body_start;
            if (body_received < static_cast<size_t>(content_length)) continue; // Need more data
        }
        return request;
    }
}

void ClientManager::processClientRequestPoll(const std::vector<int>& readable_fds)
{
    HttpRequest request;
    std::string response;

    for (size_t i = 0; i < readable_fds.size(); ++i) {
        int socket_fd = readable_fds[i];

        // Read complete HTTP request
        std::string raw_request = readFullRequest(socket_fd);
        if (raw_request.empty()) {
            removeClient(socket_fd);
            continue;
        }

        std::cout << "RAW REQUEST" << std::endl;
        std::cout << raw_request << std::endl;
        
        // Parse request and generate response
        if (request.parseRequest(raw_request, this->config.root)) {
            response = HttpResponse::createResponse(request, this->config);
        } else {
            response = "HTTP/1.0 400 Bad Request\r\n\r\n";
        }

        // Send response to client
        if (send(socket_fd, response.c_str(), response.length(), 0) < 0) {
            std::cerr << "Send failed for socket " << socket_fd << std::endl;
            removeClient(socket_fd);
            continue;
        }
        
        updateActivity(socket_fd);
    }
}

void ClientManager::sendHttpResponse(int socket_fd) {
    const std::string http_response = 
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
    
    while (it != clients.end()) {
        if (current_time - it->second.last_activity > CLIENT_TIMEOUT) {
            printf("Client timeout: socket fd %d\n", it->first);
            int socket_fd = it->first;
            ++it; // Move iterator before removal
            removeClient(socket_fd);
        } else {
            ++it;
        }
    }
}

