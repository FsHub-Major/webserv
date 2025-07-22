// Minimal HTTP server in C++ using BSD sockets

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <string>
#include <fstream>


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
    {

        cfg_path = "./server.conf";
    }
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

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
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

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on port " << port << "...\n";

    while (true) {
        sockaddr_in client_addr ;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        // Read request (not fully parsing here)
        char buffer[1024];
        ssize_t bytes = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = '\0';
            std::cout << "Request:\n" << buffer << "\n";
        }

        // Prepare simple HTTP response
        const char* response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 25\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Hello from C++ HTTP server!";

        // Send response
        send(client_fd, response, strlen(response), 0);

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
