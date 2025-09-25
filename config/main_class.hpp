#ifndef ParsingConfig_HPP
#define ParsingConfig_HPP
#include "server_config.hpp"
#include <string>
#include <map>
#include <vector>
class ParsingServer {
    private:
        std::vector<ServerConfig> servers;
        std::vector<std::string> tokens;
        size_t pos;
        std::string peek() const;
        std::string get();
        ServerConfig parseServer();
        Location parseLocation(const std::string& path);

    public:
        ParsingServer(const std::string& filename);
        void parse();  // main function to build _config from file
        const std::vector<ServerConfig>& getServers() const;

};

#endif