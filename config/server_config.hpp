
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
    std::string _path;
    std::vector<std::string> _methods;
    bool _autoindex;
    std::map<std::string, std::vector<std::string> > _directives;
public:
    Location() : _autoindex(false) {}
    void setPath(const std::string& path) { _path = path; }
    const std::string& getPath() const { return _path; }
    void setMethods(const std::vector<std::string>& methods) { _methods = methods; }
    const std::vector<std::string>& getMethods() const { return _methods; }
    void setAutoindex(bool autoindex) { _autoindex = autoindex; }
    bool getAutoindex() const { return _autoindex; }
    void addDirective(const std::string& key, const std::vector<std::string>& values) {
        _directives[key] = values;
    }
    bool hasDirective(const std::string& key) const {
        return _directives.find(key) != _directives.end();
    }
    const std::vector<std::string>& getDirective(const std::string& key) const {
    static std::vector<std::string> empty;
    std::map<std::string, std::vector<std::string> >::const_iterator it = _directives.find(key);
    if (it != _directives.end()) return it->second;
        return empty;
    }
};

class ServerConfig {
private:
    std::vector<std::pair<std::string, int> > _listen; // host + port
    size_t _clientMaxBodySize;
    std::string _serverName;
    std::vector<Location> _locations;
    std::map<std::string, std::vector<std::string> > _directives;

public:
    ServerConfig() : _clientMaxBodySize() {}
    void addListen(const std::string& host, int port) { _listen.push_back(std::make_pair(host, port)); }
    const std::vector<std::pair<std::string, int> >& getListen() const { return _listen; }
    void setServerName(const std::string& name) { _serverName = name; }
    const std::string& getServerName() const { return _serverName; }
    void setMaxBodySize(size_t size) { _clientMaxBodySize = size; }
    size_t getMaxBodySize() const { return _clientMaxBodySize; }
    void addLocation(const Location& loc) { _locations.push_back(loc); }
    const std::vector<Location>& getLocations() const { return _locations; }
    void addDirective(const std::string& key, const std::vector<std::string>& values) {
        _directives[key] = values;
    }
    bool hasDirective(const std::string& key) const {
        return _directives.find(key) != _directives.end();
    }
    const std::vector<std::string>& getDirective(const std::string& key) const {
        static std::vector<std::string> empty;
        std::map<std::string, std::vector<std::string> >::const_iterator it = _directives.find(key);
        if (it != _directives.end()) return it->second;
        return empty;
    }
};
#endif