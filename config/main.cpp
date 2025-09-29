#include "tokenizer.hpp"
#include "server_config.hpp"
#include "main_class.hpp"
#include <iostream>
#include <string>
#include <vector>

int main() {
    try {
        // Create parser and parse the config file
        ParsingServer parser("real_config_example.config");
        parser.parse();
        
        // Get all parsed servers
        const std::vector<ServerConfig>& servers = parser.getServers();
        
        std::cout << "=== Parsed Configuration ===" << std::endl;
        std::cout << "Number of servers: " << servers.size() << std::endl << std::endl;
        
        // Print each server's configuration
        for (size_t i = 0; i < servers.size(); ++i) {
            const ServerConfig& server = servers[i];
            
            std::cout << "--- Server " << (i + 1) << " ---" << std::endl;
            
            // Print listen addresses
            const std::vector<std::pair<std::string, int> >& listen = server.getListen();
            for (size_t j = 0; j < listen.size(); ++j) {
                std::cout << "Listen: " << listen[j].first << ":" << listen[j].second << std::endl;
            }
            
            // Print server name
            if (!server.getServerName().empty()) {
                std::cout << "Server Name: " << server.getServerName() << std::endl;
            }
            
            // Print max body size
            std::cout << "Max Body Size: " << server.getMaxBodySize() << std::endl;
            
            // Print locations
            const std::vector<Location>& locations = server.getLocations();
            std::cout << "Locations (" << locations.size() << "):" << std::endl;
            for (size_t k = 0; k < locations.size(); ++k) {
                const Location& loc = locations[k];
                std::cout << "  Path: " << loc.getPath() << std::endl;
                
                // Print methods
                const std::vector<std::string>& methods = loc.getMethods();
                if (!methods.empty()) {
                    std::cout << "    Methods: ";
                    for (size_t m = 0; m < methods.size(); ++m) {
                        std::cout << methods[m];
                        if (m < methods.size() - 1) std::cout << ", ";
                    }
                    std::cout << std::endl;
                }
                
                // Print autoindex
                std::cout << "    Autoindex: " << (loc.getAutoindex() ? "on" : "off") << std::endl;
            }
            std::cout << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Config parsing completed successfully!" << std::endl;
    return 0;
}


