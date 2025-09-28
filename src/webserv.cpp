/**
 * @file webserv.cpp
 * @brief Main entry point for the webserv HTTP server
 * 
 * A minimal HTTP server implementation using BSD sockets with platform-specific
 * event monitoring (poll on macOS, epoll on Linux) for handling multiple clients.
 */

#include "webserv.hpp"
#include "macros.hpp"
#include "Config.hpp"
#include "Server.hpp"


int load_port(const std::string& path)
{
    std::ifstream cfg(path.c_str());
    if (!cfg.is_open()) {
        std::cerr << "Failed to open config file: " << path << "\n";
        return -1;
    }
    
    std::vector<std::string> tokens;
    tokens.reserve(10); // Pre-allocate to avoid reallocations
    std::string line;
    
    while (std::getline(cfg, line)) {
        tokens = split(line, " =");
        for (size_t i = 0; i + 1 < tokens.size(); ++i) {
            if (tokens[i] == "port") {
                return stringtoi(tokens[i + 1]);
            }
        }
        tokens.clear(); // Clear for next line
    }
    
    std::cerr << "No port setting found in config file\n";
    return -1;
}

ServerConfig initializeServerConfig()
{
    ServerConfig server_config;
    server_config.port = 8080;
    server_config.server_name = "localhost";
    server_config.root = "./www";
    server_config.index_files.push_back("index.html");
    server_config.client_timeout = CLIENT_TIMEOUT;
    server_config.client_max_body_size = 1024 * 1024; // 1MB default
    
    return server_config;
}


int main(int ac, char* av[])
{
    (void)av; // Suppress unused parameter warnings
    (void)ac;
    
    try
    {
        ServerConfig server_config = initializeServerConfig();
        
        Server server(server_config);
        if (!server.init()) { 
            std::cerr << "Failed to initialize server on port " << server_config.port << std::endl;
            return 1;
        }
        
        std::cout << "Server initialized successfully" << std::endl;
        server.run(); // Main event loop
    } 
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Server shutdown completed successfully" << std::endl;
    return 0;
}
