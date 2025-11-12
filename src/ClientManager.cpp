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
#include "ext_libs.hpp"

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
    new_client.recv_buffer.clear();
    
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
        it->second.recv_buffer.clear();
        clients.erase(it);
    }
}


int ClientManager::getClientCount() const
{
    return static_cast<int>(clients.size());
}

bool ClientManager::isFull() const {
    return clients.size() >= MAX_CLIENTS;
}

std::string ClientManager::readFullRequest(int socket_fd)
{
    // Legacy blocking reader retained for compatibility if needed elsewhere.
    // Not used in the incremental poll path below.
    std::string req;
    char buf[BUFF_SIZE];
    while (true)
    {
        int n = read(socket_fd, buf, sizeof(buf));
        if (n <= 0)
            break;
        req.append(buf, n);
        if (requestComplete(req))
            break;
    }
    return req;
}

bool ClientManager::readPartial(int socket_fd, std::string &buffer, size_t max_bytes)
{
    char tmp[4096];
    size_t to_read = (max_bytes < sizeof(tmp)) ? max_bytes : sizeof(tmp);
    int n = recv(socket_fd, tmp, to_read, 0);
    if (n > 0)
    {
        buffer.append(tmp, n);
        return true;
    }
    if (n == 0) // connection closed by peer
        return (false);
    // For n < 0, we avoid errno checks; treat as transient and keep socket.
    // The poller will notify again if there's more to read or an error.
    return (true);
}

static size_t parse_content_length(const std::string &headers)
{
    size_t cl_pos = headers.find("Content-Length:");
    size_t hdr_end = headers.find("\r\n\r\n");
    size_t val_start = cl_pos + 15;
    size_t val_end = headers.find("\r\n", val_start);
    int len = stringtoi(trim(headers.substr(val_start, val_end - val_start), " \t\r\n"));

    if (cl_pos == std::string::npos)
        return (size_t)-1;
    if (hdr_end == std::string::npos)
        return (size_t)-1;
    while (val_start < hdr_end && (headers[val_start] == ' ' || headers[val_start] == '\t'))
        ++val_start;
    if (val_end == std::string::npos || val_end > hdr_end) val_end = hdr_end;
    if (len < 0) len = 0;
    return (static_cast<size_t>(len));
}

bool ClientManager::requestComplete(const std::string &buffer)
{
    size_t need = parse_content_length(buffer);
    size_t hdr_end = buffer.find("\r\n\r\n");
    size_t body_start = hdr_end + 4;
    size_t cl_pos = buffer.find("Content-Length:");

    if (hdr_end == std::string::npos)
        return (false);
    // Check for Content-Length
    if (cl_pos == std::string::npos || cl_pos > hdr_end)
        return (true); // no body
    if (need == (size_t)-1)
        return (true); // treat as no body if can't parse
    return (buffer.size() - body_start >= need);
}

// poll variant: readable_fds are those with POLLIN ready
void ClientManager::processClientRequestPoll(const std::vector<int>& readable_fds)
{
    // Small per-iteration read budget per client
    const size_t kChunk = 4096; // 4KB per readiness event
    HttpRequest request;
    std::string response;

    for (size_t i = 0; i < readable_fds.size(); ++i) {
        int socket_fd = readable_fds[i];
        std::map<int, Client>::iterator it = clients.find(socket_fd);
        if (it == clients.end())
            continue;

        Client &cli = it->second;

        // Read a small chunk and return to event loop quickly
        if (!readPartial(socket_fd, cli.recv_buffer, kChunk))
        {
            removeClient(socket_fd);
            continue;
        }

        // Only proceed when full request is available
        if (!requestComplete(cli.recv_buffer))
        {
            updateActivity(socket_fd);
            continue; // wait for more data
        }

        // Parse and respond
        if (request.parseRequest(cli.recv_buffer, this->config.root))
            response = HttpResponse::createResponse(request, this->config);
        else
            response = "HTTP/1.0 400 Bad Request\r\n\r\n";

        if (send(socket_fd, response.c_str(), response.length(), 0) < 0)
        {
            std::cerr << "Send failed for socket " << socket_fd << std::endl;
            removeClient(socket_fd);
            continue;
        }
        updateActivity(socket_fd);

        // Clear buffer for next request on keep-alive; if expecting persistent
        // connections, consider parsing only consumed bytes. For now, we clear.
        cli.recv_buffer.clear();
    }
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
            ++it;
            removeClient(socket_fd);
        } else {
            ++it;
        }
    }
}

