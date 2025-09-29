

#include "Server.hpp"
#include "macros.hpp"
#include "debug.hpp"
#include <poll.h>

Server::Server(const ServerConfig & config)
        : config(config), clients(config), server_fd(-1) ,is_running(false), is_init(false)
{
    std::memset(&address, 0 , sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost
    // address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(config.port);
}

Server::~Server()
{
    cleanup();
}

bool Server::init()
{
    if (is_init)
        return (true);
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket creation failed");
        return (false);
    }

    // Add SO_REUSEADDR to prevent "Address already in use" errors
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return false;
    }

    if (::bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return (false);
    }

    // 128 backlog queue (pending clients) , not MAX_CLIENTS of concurrent clients
    if (listen(server_fd, 128) == -1) {
        perror("listen");
        close(server_fd);
        return (false);
    } 
    is_init = true;
    std::cout << "Server listening on port " << config.port << "...\n";
    return (true);
}


void Server::buildPollFds()
{
    poll_fds.clear();
    struct pollfd pfd;
    pfd.fd = server_fd;
    pfd.events = POLLIN; // watch for incoming connections
    pfd.revents = 0;
    poll_fds.push_back(pfd);

    // add client sockets
    for (std::map<int, Client>::const_iterator it = clients.clientsBegin(); it != clients.clientsEnd(); ++it)
    {
        struct pollfd cfd;
        cfd.fd = it->first;
        cfd.events = POLLIN; // only read for now
        cfd.revents = 0;
        poll_fds.push_back(cfd);
    }
}

void Server::run() {
    if (!is_init) {
        std::cerr << "Server not initialized" << std::endl;
        return;
    }
    is_running = true;

    std::cout << "Server running on port " << config.port << std::endl;
    while (is_running)
    {
        buildPollFds();

        // If no fds to poll, sleep briefly and continue
        if (poll_fds.empty()) {
            usleep(10000); // 10ms
            continue;
        }

        // poll timeout in milliseconds (5s)
        int ret = poll(&poll_fds[0], poll_fds.size(), 5000);
        if (ret < 0)
        {
            if (errno == EINTR)
                continue; // interrupted by signal
            perror("poll failed");
            break;
        }
        if (ret == 0)
        {
            // timeout: still do timeout checks
            clients.checkTimeouts();
            continue;
        }
        processReadyFds();
    }
    std::cout << "Server stopped : " << config.server_name << std::endl;
}

bool Server::handleNewConnection()
{
    if (!is_running || server_fd == -1)
        return (false);
    
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int new_socket = accept(server_fd, (struct sockaddr*) &client_addr, &client_len);
    if (new_socket < 0)
    {
        perror("accept failer");
        return (false);
    }
    printf("New connection: socket fd is %d, IP is %s, port %d\n",
           new_socket, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    if (!clients.addClient(new_socket, client_addr))
    {
        std::cerr << "Failed to add client - server full" << std::endl;
        close(new_socket);
        return (false);
    }
    return (true); 
}

void Server::processReadyFds()
{
    // index 0 is server fd
    if (poll_fds.empty())
        return;
    if (poll_fds[0].revents & POLLIN)
    {
        handleNewConnection();
    }
    // collect readable client fds
    std::vector<int> readable;
    for (size_t i = 1; i < poll_fds.size(); ++i)
    {
        if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            clients.removeClient(poll_fds[i].fd);
            continue;
        }
        if (poll_fds[i].revents & POLLIN)
            readable.push_back(poll_fds[i].fd);
    }
    if (!readable.empty())
        clients.processClientRequestPoll(readable); // new API
    clients.checkTimeouts();
}


void Server::cleanup()
{
    if (server_fd != -1)
    {
        close(server_fd);
        server_fd = -1;
    }
    is_running = false;
    is_init = false;
}

int Server::getPort() const {
    return config.port;
}

int Server::getServerFd() const {
    return server_fd;
}

bool Server::isRunning() const {
    return is_running;
}

void Server::stop()
{
    is_running = false;
}

bool Server::isInitialized() const {
    return is_init;
}

