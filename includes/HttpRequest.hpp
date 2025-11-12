#pragma once
#include "ext_libs.hpp"

class HttpRequest
{
private:
    // Request line components
   std::string                         method;          // GET, POST, DELETE. We only need 3
   std::string                         uri;            // /path/to/resource
   std::string                         httpVersion;    // HTTP/1.1
   std::string                         root;           //path to root server

    // Headers
   std::map<std::string, std::string>  headers;
    
    // Body (for POST requests)
   std::string                         body;
    
    // Query parameters (from URL)
   std::map<std::string, std::string>  queryParams;
  
    
    // Raw request data
   std::string rawRequest;
 public:
    bool    isQuery;
    // Constructor
   HttpRequest();
    
    // Main parsing function
   bool                                      parseRequest(const std::string& rawRequest, 
                                                const std::string root);
    
    // Utility functions
   void                                      printRequest() const;

    //getters
   const std::string                         &getMethod() const;
   const std::string                         &getUri() const;
   const std::string                         &getHttpVersion() const;
   const std::map<std::string,std::string>   &getHeaders() const;
   const std::string                         &getHeader(const std::string& key) const;
   const std::string                         &getRoot() const;
   const std::string                         &getBody() const;
 private:
   void  parseQuery();
};