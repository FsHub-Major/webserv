#ifndef ParsingConfig_HPP
#define ParsingConfig_HPP
#include "server_config.hpp"
#include <string>
#include <map>
#include <vector>
class ParsingServer {
    private:
        std::vector<ServerConfig> _servers;

    public:
        ParsingServer(const std::string& filename);
        void parse();  // main function to build _config from file
        const std::vector<ServerConfig>& getServers() const;

};

#endif