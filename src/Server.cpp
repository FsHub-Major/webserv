

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
        return (false);
    }
    // 128 backlog queue (pending clients) , not MAX_CLIENTS of concurrent clients
    if (listen(server_fd, 128) == -1) {
        perror("listen");
        close(server_fd);
        return (false);
    } 
    is_init = true;
    std::cout << "Server listening on port " << port << "...\n";
    return (true);
}

void Server::run() {
    if (!is_init) {
        std::cerr << "Server not initialized" << std::endl;
        return;
    }
    is_running = true;
}

bool Server::handleNewConnection()
{
    if (!is_running || server_fd == -1)
        return (false);
    
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int new_socket = accept(server_fd, (struct sockaddr*) &client_addr, &client_len);
    if (!new_socket < 0)
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
    }
    return (true); 
}



   while (true) {
        struct timeval timeout;
        timeout.tv_sec = 5; // check every 5 sec
        timeout.tv_usec = 0;

        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        time_t current_time = time(NULL);
        // Add client sockets to the read set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0)
            {
                if (current_time - client_last_activity[i] > CLIENT_TIMEOUT)
                {
                    printf("Client timeout: socket fd %d\n", sd);
                    close(sd);
                    client_sockets[i] = 0;
                    client_last_activity[i] = 0;
                }
                else
                {
                    FD_SET(sd, &readfds);
                    if (sd > max_sd) max_sd = sd;
                }
            } 
        }

        // blocking Wait for activity on any socket: Event-driven (CPU sleeps)
        int activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0) {
            perror("select");
            break;
        }

        // Handle new conneciton 
        if (FD_ISSET(server_fd, &readfds)) {
            // 5. New connection
            new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
            if (new_socket >= 0)
            {
                printf("New connection: socket fd is %d, IP is %s, port %d\n",
                    new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                // Add to list of clients
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (client_sockets[i] == 0) {
                        client_sockets[i] = new_socket;
                        client_last_activity[i] = time(NULL);
                        break;
                    }
                }
            }
            else{
                perror("accept failed");
            }
        } 
         // 6. Handle client data : Check each client for incoming data
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                char buffer[1024] = {0};
                int valread = read(sd, buffer, sizeof(buffer));
                if (valread == 0) {
                    // Connection closed
                    getpeername(sd, (struct sockaddr*)&address, &addrlen);
                    printf("Client disconnected: IP %s, port %d\n",
                           inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close(sd);
                    client_sockets[i] = 0;
                    client_last_activity[i] = 0;
                } else {
                    // Send HTTP response instead of echo
                    buffer[valread] = '\0';
                    std::string http_response = 
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: 13\r\n"
                        "\r\n"
                        "Hello World!";
                    send(sd, http_response.c_str(), http_response.length(), 0);
                    client_last_activity[i] = time(NULL);  // Update activity time
                }
            }
        }
    } 

    close(server_fd);



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

