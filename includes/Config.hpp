#pragma once

# include <ext_libs.hpp>

struct LocationConfig
{
    std::string location;
    std::string path;
    std::vector<std::string> allowed_methods;
    bool autoindex;
    std::string upload_dir;
    std::vector<std::string> cgi_extensions;
    std::string cgi_path;
    std::string fastcgi_pass; // host:port or unix socket path
    bool has_return;
    int return_code;
    std::string return_target;
};

struct ServerConfig
{
    int port;
    std::string server_name;
    std::string root;
    std::vector<std::string> index_files;
    std::map<int, std::string> error_pages;
    size_t client_max_body_size;
    int client_timeout;
    std::vector<LocationConfig> locations;
};

class Config
{
private:
    std::vector<ServerConfig> servers;
    
public:
    Config(const std::string& config_path);
    const std::vector<ServerConfig>& getServers() const { return servers; }
    const ServerConfig* getServerByPort(int port) const;
    void printDebug(std::ostream& os) const;
    
private:
    void parseConfigFile(const std::string& path);
    void parseGlobalDirective(ServerConfig& server_config,
                             const std::vector<std::string>& tokens);
    ServerConfig parseServerBlock(std::ifstream& file, const ServerConfig& global_config);
    LocationConfig parseLocationBlock(std::ifstream& file,
                                     const std::string& location_path);
    void parseLocationDirective(LocationConfig& location,
                               const std::vector<std::string>& tokens);
    void validateServerBlockCompletion(bool has_listen, bool has_explicit_root,
                                      bool found_closing_brace, const std::string& root);
    
    // Utility methods
    std::vector<std::string> split(const std::string& str, char delimiter);
    void trim(std::string& str);
    std::vector<std::string> splitTokens(const std::string& statement);
    std::string removeTrailingSemicolon(std::string line);
    bool endsWithBrace(const std::string& line);
    std::string stripInlineComment(const std::string& line);
    
    // Token parsing
    int parsePortToken(const std::string& token);
    size_t parseSizeToken(const std::string& token);
    bool parseBoolToken(const std::string& value);
    
    // Validation
    bool isKnownServerDirective(const std::string& directive);
    void validateDirectiveValue(const std::string& directive,
                               const std::vector<std::string>& tokens);
    
    // Configuration
    ServerConfig createDefaultServerConfig();

    // Parsing helpers (split from parseServerBlock)
    std::string collectHeader(std::ifstream& file, const std::string& header);
    bool isLocationHeader(const std::string& current);
    void handleLocationHeader(std::ifstream& file, ServerConfig& server, const std::string& header);
    void processServerDirective(ServerConfig& server, const std::string& directive,
                                const std::vector<std::string>& tokens, bool& has_listen,
                                bool& has_explicit_root);
};
