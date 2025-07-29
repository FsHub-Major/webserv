

#pragma once

#include <string>
#include <vector>
#include <map>

struct LocationConfig {
    std::string path;
    std::vector<std::string> allowed_methods;
    bool autoindex;
    std::string upload_dir;
    std::vector<std::string> cgi_extensions;
    std::string cgi_path;
};

struct ServerConfig {
    int port;
    std::string server_name;
    static std::string root;
    std::vector<std::string> index_files;
    std::map<int, std::string> error_pages;
    size_t client_max_body_size;
    int client_timeout;
    std::vector<LocationConfig> locations;
};

class Config {
private:
    std::vector<ServerConfig> servers;
    
public:
    Config(const std::string& config_path);
    const std::vector<ServerConfig>& getServers() const { return servers; }
    const ServerConfig* getServerByPort(int port) const;
    
private:
    void parseConfigFile(const std::string& path);
    ServerConfig parseServerBlock(std::ifstream& file, std::string& line);
    LocationConfig parseLocationBlock(std::ifstream& file, const std::string& location_path);
    std::vector<std::string> split(const std::string& str, char delimiter);
    void trim(std::string& str);
};
