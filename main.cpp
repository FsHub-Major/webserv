// Minimal HTTP server in C++ using BSD sockets

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <cstdio> 
#include <time.h> // time functions 
#include <sys/time.h> //gettimeofday

#define MAX_CLIENTS 20
#define CLIENT_TIMEOUT 30 // in sec

int load_port(const std::string& path) {
    std::ifstream cfg(path);
    if (!cfg.is_open()) {
        std::cerr << "Failed to open config file: " << path << "\n";
        return -1;
    }
    std::string line;
    while (std::getline(cfg, line)) {
        if (line.rfind("port=", 0) == 0) {
            try {
                return std::stoi(line.substr(5));
            } catch (...) {
                std::cerr << "Invalid port value in config: " << line << "\n";
                return -1;
            }
        }
    }
    std::cerr << "No port setting found in config file\n";
    return -1;
}

int main(int ac, char *av[]) {

    std::string cfg_path;
    if (ac > 1)
    {
        cfg_path = av[1];
    }else
        cfg_path = "./server.conf";
    std::ifstream test_file(cfg_path.c_str());
    if (!test_file.is_open()) {
        std::cerr << "Configuration file not found: " << cfg_path << "\n";
        return 1;
    }
    test_file.close();
 
    const int port = load_port(cfg_path);

    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port loaded: " << port << "\n";
        return 1;
    }
    std::cout << "Using port " << port << " from config: " << cfg_path << "\n";

    // fds for server + clients  
    int server_fd, new_socket, client_sockets[MAX_CLIENTS];
    time_t client_last_activity[MAX_CLIENTS];
    fd_set readfds;


    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        client_sockets[i] = 0;
        client_last_activity[i] = 0;
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    sockaddr_in address ;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    socklen_t addrlen = sizeof(address);

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

    std::cout << "Server listening on port " << port << "...\n";

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
                    // Echo message back
                    buffer[valread] = '\0';
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    } 

    close(server_fd);
    return 0;
}
