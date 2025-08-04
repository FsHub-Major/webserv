

#include "Server.hpp"
#include "macros.hpp"


#include "debug.hpp"

Server::Server(const ServerConfig & config)
        : config(config), clients(), server_fd(-1) ,is_running(false), is_init(false)
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


    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
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


void Server::setupSelectFds(fd_set* readfds, int* max_fd)
{
    if (!readfds || !max_fd)
        return ;
    
    FD_SET(server_fd, readfds);
    if (server_fd > *max_fd){
        *max_fd = server_fd;
    }
    clients.setupClientFds(readfds, max_fd);
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
        fd_set readfds;
        int max_fd = server_fd;

        setupSelectFds(&readfds, &max_fd);

        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0)
        {
            if (errno == EBADF)
            {
                // check avaible fds to see if deleted 
                Debug::inspectReadfds(&readfds, max_fd, server_fd);
                clients.cleanupInvalidFds(&readfds);
                continue;
            }
            else if (errno == EINTR)
            {
                continue;
            }
            else if (is_running)
            {
                perror("select failed");
                break; // should exit 
            }
            break;
        }
        if (activity > 0)
        {
            if (FD_ISSET(server_fd, &readfds))
                handleNewConnection();
            processRequest(&readfds);
        }
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

void Server::processRequest(fd_set* readfds)
{
    if (!readfds) return ;

    clients.processClientRequest(readfds);
    // clients.checkTimeouts();
};


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

