

#pragma once

#include "Config.hpp"
#include "ext_libs.hpp"
#include "macros.hpp"
#include <map>
#include <vector>

struct Client {
    Client()
        : socket_fd(-1)
        , last_activity(0)
        , address()
        , is_active(false)
        , recv_buffer()
    {
    }

    int socket_fd;
    time_t last_activity;
    struct sockaddr_in address;
    bool is_active;
    // Incremental request buffer for this client
    std::string recv_buffer;
};

class ClientManager
{
private:
    const ServerConfig& config;
    std::map<int, Client> clients;
    enum ReadResult
    {
        READ_OK,
        READ_CLOSED,
        READ_ERROR
    };
    
public:
    ClientManager(const ServerConfig & config);
    ~ClientManager();
    
    bool addClient(int socket_fd, const struct sockaddr_in& addr);
    void removeClient(int socket_fd);
    
    void updateActivity(int socket_fd);
    void checkTimeouts();
    
    // poll based processing
    void processClientRequestPoll(const std::vector<int>& readable_fds);

    // iteration access for building pollfd list
    std::map<int, Client>::const_iterator clientsBegin() const { return clients.begin(); }
    std::map<int, Client>::const_iterator clientsEnd() const { return clients.end(); }
    
    int getClientCount() const;
    bool isFull() const;
    
private:
    std::string readFullRequest(int socket_fd);

    // Incremental reading helpers
    ReadResult readPartial(int socket_fd, std::string &buffer, size_t max_bytes);
    bool requestComplete(const std::string &buffer);
    // Contract: requestComplete returns true when headers found and either
    //  - no Content-Length in headers, or
    //  - body bytes >= parsed Content-Length
    // chunked transfer-encoding is not handled here.

};
