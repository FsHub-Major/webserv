
#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP
#include "tokenizer.hpp"
// create the server class
#include <string>
#include <map>
#include <vector>
// requirements
//server block
//location block
// CGI block

class Location {
    private:
        std::string path;
        std::string root;
        std::vector<std::string> methods;
        bool autoindex;
        std::string indexFile;
        std::string uploadStore;
        std::map<std::string, std::string> _cgi;
    public:
        //getters and setters
        bool isMethodAllowed(const std::string& method) const;
};

class ServerConfig {
    private:
        std::string host;
        int port;
        std::map<int, std::string> errorPages;
        size_t clientMaxBodySize;
        std::vector<Location> locations;
    public:
    void setHost(const std::string& host);
    void setPort(int port);
    void addErrorPage(int code, const std::string& path);
    void setMaxBodySize(size_t size);
    void addRoute(const Location& route);

    const std::string& getHost() const;
    int getPort() const;
    const std::map<int, std::string>& getErrorPages() const;
    size_t getMaxBodySize() const;
    const std::vector<Location>& getRoutes() const;
        void addLocation(const Location& loc);
        const std::vector<Location>& getLocations() const;
};



#endif