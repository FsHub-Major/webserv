
#include "Server.hpp"
#include "macros.hpp"
#include "debug.hpp"

Server::Server(const ServerConfig & config)
        : config(config), clients(config), server_fd(-1), is_running(false), is_init(false)
{
    // Initialize socket address structure for IPv4
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1"); // Bind to localhost only
    address.sin_port = htons(config.port);
    
#ifdef __linux__
    epoll_fd = -1; // Initialize epoll file descriptor for Linux
#endif
}

Server::~Server()
{
    cleanup();
}


bool Server::init()
{
    if (is_init)
        return true;
    
    // Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket creation failed");
        return (false);
    }
    // Enable address reuse to prevent "Address already in use" errors
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt failed");
        close(server_fd);
        return false;
    }

    // Bind socket to the configured address and port
    if (::bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1)
    {
        perror("bind");
        close(server_fd);
        return false;
    }

    // Start listening with backlog queue size of 128 (pending connections)
    if (listen(server_fd, 128) == -1)
    {
        perror("listen");
        close(server_fd);
        return false;
    }
    
#ifdef __linux__
    // Initialize epoll for efficient event monitoring on Linux
    if (!initEpoll()) {
        close(server_fd);
        return false;
    }
#endif
    
    is_init = true;
    std::cout << "Server listening on port " << config.port << "...\n";
    return true;
}

#ifdef __linux__

bool Server::initEpoll()
{
    epoll_fd = epoll_create(1);
    if (epoll_fd < 0)
    {
        std::cerr << "epoll_create failed" << std::endl;
        return false;
    }
    
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) < 0)
    {
        std::cerr << "epoll_ctl ADD server_fd failed" << std::endl;
        close(epoll_fd);
        epoll_fd = -1;
        return false;
    }
    
    return true;
}
#endif



void Server::buildPollFds()
{
    poll_fds.clear();
    
    // Add server socket for new connections
    struct pollfd server_pfd = {server_fd, POLLIN, 0};
    poll_fds.push_back(server_pfd);

    // Add all client sockets for reading
    for (std::map<int, Client>::const_iterator it = clients.clientsBegin(); 
         it != clients.clientsEnd(); ++it)
    {
        struct pollfd client_pfd = {it->first, POLLIN, 0};
        poll_fds.push_back(client_pfd);
    }
}

void Server::run() {
    if (!is_init)
    {
        std::cerr << "Server not initialized" << std::endl;
        return;
    }
    is_running = true;
    std::cout << "Server running on port " << config.port << std::endl;

#ifdef __APPLE__
    runWithPoll();
#endif
#ifdef __linux__
    runWithEpoll();
#endif
    
    std::cout << "Server stopped : " << config.server_name << std::endl;
}

#ifdef __APPLE__
/**
 * @brief Event loop implementation using poll() for macOS
 */
void Server::runWithPoll()
{
    const int TIMEOUT_MS = 5000; // 5 second timeout
    
    while (is_running)
    {
        buildPollFds();

        int ret = poll(&poll_fds[0], poll_fds.size(), TIMEOUT_MS);
        if (ret < 0)
        {
            std::cerr << "poll failed" << std::endl;
            break;
        }
        
        if (ret == 0)
        {
            // Timeout - check for inactive clients
            clients.checkTimeouts();
            continue;
        }
        
        processReadyFds();
    }
}
#endif

#ifdef __linux__

void Server::runWithEpoll()
{
    const int MAX_EVENTS = 1024;
    const int TIMEOUT_MS = 5000; // 5 second timeout
    struct epoll_event events[MAX_EVENTS];
    int n;
    
    while (is_running)
    {
        n = epoll_wait(epoll_fd, events, MAX_EVENTS, TIMEOUT_MS);
        if (n < 0)
        {
            std::cerr << "epoll_wait failed" << std::endl;
            break;
        }
        
        if (n == 0)
        {
            clients.checkTimeouts();
            continue;
        }

        processEpollEvents(events, n);
        clients.checkTimeouts();
    }
}


void Server::processEpollEvents(struct epoll_event* events, int count)
{
    std::vector<int> readable;
    int fd;

    for (int i = 0; i < count; ++i)
    {
        fd = events[i].data.fd;
        uint32_t event_flags = events[i].events;
        
        if (fd == server_fd && (event_flags & EPOLLIN))
        {
            handleNewConnection();
            continue;
        }
        
        if (event_flags & (EPOLLERR | EPOLLHUP))
        {
            clients.removeClient(fd);
            continue;
        }
        
        if (event_flags & EPOLLIN)
            readable.push_back(fd);
    }
    
    if (!readable.empty()) {
        clients.processClientRequestPoll(readable);
    }
}
#endif

bool Server::handleNewConnection()
{
    if (!is_running || server_fd == -1)
        return false;
    
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int new_socket = accept(server_fd, (struct sockaddr*) &client_addr, &client_len);
    if (new_socket < 0)
    {
        perror("accept failed");
        return false;
    }
    
    printf("New connection: socket fd %d, IP %s, port %d\n",
           new_socket, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    // Try to add client to manager
    if (!clients.addClient(new_socket, client_addr))
    {
        std::cerr << "Failed to add client - server full" << std::endl;
        close(new_socket);
        return false;
    }
    
#ifdef __linux__
    // Register new client socket with epoll
    registerClientWithEpoll(new_socket);
#endif
    return true;
}

#ifdef __linux__

void Server::registerClientWithEpoll(int socket_fd)
{
    if (epoll_fd >= 0)
    {
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = socket_fd;
        
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &ev) < 0)
        {
            std::cerr << "epoll_ctl ADD client failed" << std::endl;
            // Continue anyway; read loop will handle failure and cleanup
        }
    }
}
#endif


void Server::processReadyFds()
{
    if (poll_fds.empty())
        return;
        
    // Check server socket for new connections (index 0)
    if (poll_fds[0].revents & POLLIN)
        handleNewConnection();
    
    // Process client sockets (starting from index 1)
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
    
    if (!readable.empty()) {
        clients.processClientRequestPoll(readable);
    }
    
    clients.checkTimeouts();
}



void Server::cleanup()
{
    if (server_fd != -1)
    {
        close(server_fd);
        server_fd = -1;
    }
    
#ifdef __linux__
    if (epoll_fd != -1)
    {
        close(epoll_fd);
        epoll_fd = -1;
    }
#endif
    
    is_running = false;
    is_init = false;
}

// Getter methods
int Server::getPort() const
{
    return config.port;
}

int Server::getServerFd() const
{
    return server_fd;
}

bool Server::isRunning() const
{
    return is_running;
}

bool Server::isInitialized() const
{
    return is_init;
}

void Server::stop()
{
    is_running = false;
}

