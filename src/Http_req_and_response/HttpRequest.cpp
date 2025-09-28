/**
 * @file HttpRequest.cpp
 * @brief HTTP request parsing and handling implementation
 */

#include "HttpRequest.hpp"
#include "ext_libs.hpp"


HttpRequest::HttpRequest(void)
{
    isQuery = false;
}


void HttpRequest::parseQuery(void)
{
    vector<std::string> query_pairs;
    std::string key, val;

    // Extract query string after '?' and split by '&'
    query_pairs = split(uri.substr(uri.find_first_of('?') + 1), "&");
    
    for (std::size_t i = 0; i < query_pairs.size(); i++) {
        size_t eq_pos = query_pairs[i].find_first_of('=');
        if (eq_pos != std::string::npos) {
            key = query_pairs[i].substr(0, eq_pos);
            val = query_pairs[i].substr(eq_pos + 1);
            queryParams[key] = val;
        }
    }
}


bool HttpRequest::parseRequest(const std::string &request, const std::string root)
{
    std::string line, key, val;

    rawRequest = request;
    this->root = root;
    
    // Parse request line (method, URI, HTTP version)
    std::istringstream req_stream(request);
    req_stream >> method >> uri >> httpVersion;

    // Parse query parameters if present in URI
    if (uri.find('?') != std::string::npos) {
        parseQuery();
        isQuery = true;
    }
    
    // Skip to next line and parse headers
    std::getline(req_stream, line);
    while (std::getline(req_stream, line) && line != "\r") {
        size_t colon_pos = line.find_first_of(':');
        if (colon_pos != std::string::npos) {
            key = line.substr(0, colon_pos);
            val = trim(line.substr(colon_pos + 1), " \r");
            headers[key] = val;
        }
    }
    
    // Read request body (everything after headers)
    while (std::getline(req_stream, line)) {
        body += line;
    }

    return true;
}


void HttpRequest::printRequest(void) const
{
    std::cout << "=== HTTP Request Details ===" << std::endl;
    
    // Print request line components
    std::cout << "Method: " << method << std::endl;
    std::cout << "Path: " << uri << std::endl;
    std::cout << "HTTP Version: " << httpVersion << std::endl << std::endl;
    
    // Print all headers
    std::cout << "Headers:" << std::endl;
    if (headers.empty()) {
        std::cout << "  (no headers)" << std::endl;
    } else {
        for (std::map<std::string, std::string>::const_iterator it = headers.begin(); 
             it != headers.end(); ++it) {
            std::cout << "  " << it->first << ": " << it->second << std::endl;
        }
    }
    std::cout << std::endl;
    
    // Print query parameters
    std::cout << "Query Parameters:" << std::endl;
    if (queryParams.empty()) {
        std::cout << "  (no query parameters)" << std::endl;
    } else {
        for (std::map<std::string, std::string>::const_iterator it = queryParams.begin(); 
             it != queryParams.end(); ++it) {
            std::cout << "  " << it->first << " = " << it->second << std::endl;
        }
    }
    std::cout << std::endl;
    
    // Print body information
    std::cout << "Body:" << std::endl;
    if (body.empty()) {
        std::cout << "  (no body)" << std::endl;
    } else {
        std::cout << "  Length: " << body.length() << " bytes" << std::endl;
        std::cout << "  Content: " << body << std::endl;
    }
    std::cout << std::endl << "=========================" << std::endl;
}

// Getter methods for accessing request data
const std::string& HttpRequest::getMethod() const {
    return method;
}

const std::string& HttpRequest::getUri() const {
    return uri;
}

const std::string& HttpRequest::getHttpVersion() const {
    return httpVersion;
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
    return headers;
}


const std::string& HttpRequest::getHeader(const std::string& key) const {
    static const std::string empty = "";
    
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    return (it != headers.end()) ? it->second : empty;
}

const std::string &HttpRequest::getRoot() const {
    return root;
}