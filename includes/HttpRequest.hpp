#pragma once
#include "ext_libs.hpp"

class HttpRequest
{
private:
    // Request line components
    std::string method;          // GET, POST, PUT, DELETE, etc.
    std::string uri;            // /path/to/resource
    std::string httpVersion;    // HTTP/1.1
    
    // Headers
    std::map<std::string, std::string> headers;
    std::string rawHeaders;
    
    // Body (for POST requests)
    std::string body;
    
    // Query parameters (from URL)
    std::map<std::string, std::string> queryParams;
    
    // Additional parsed data
    std::string host;
    int contentLength;
    std::string contentType;
    bool keepAlive;
    
    // Raw request data
    std::string rawRequest;

public:
    // Constructor
    HttpRequest();
    HttpRequest(const std::string& rawRequest);
    
    // Main parsing function
    bool parseRequest(const std::string& rawRequest);
    
    // Utility functions
    void printRequest() const;

    //getters
    const std::string &getMethod() const;
    const std::string &getUri() const;
    const std::string &getHttpVersion() const;
    const std::map<std::string, std::string>& getHeaders() const;
    const std::string &getHeader(const std::string& key) const;
};