

#include "Server.hpp"
#include "macros.hpp"




Server::Server(const ServerConfig & config)
        : config(config), clients(), server_fd(-1) ,is_running(false), is_init(false)
{
    port = config.port;
    std::memset(&address, 0 , sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

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
        return 1;
    }

    if (listen(server_fd, 128) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }
    
}

void Server::run() {
    if (!is_init) {
        std::cerr << "Server not initialized" << std::endl;
        return;
    }
    is_running = true;
}

bool Server::handleConnection()
{

}

int Server::getPort() const {
    return port;
}

int Server::getServerFd() const {
    return server_fd;
}

bool Server::isRunning() const {
    return is_running;
}

bool Server::isInitialized() const {
    return is_init;
}

