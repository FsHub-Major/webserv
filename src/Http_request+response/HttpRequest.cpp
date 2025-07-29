#include "HttpRequest.hpp"
#include "ext_libs.hpp"


HttpRequest::HttpRequest(const std::string &request)
{
    parseRequest(request);
}

HttpRequest::HttpRequest(void)
{
    isQuery = false;
}

void HttpRequest::parseQuery(void)
{

}

bool HttpRequest::parseRequest(const std::string &request)
{
    std::string line;
    std::string key;
    std::string val;

    rawRequest = request;
    std::istringstream req_stream(request);
    req_stream >> method >> uri >> httpVersion;

    if (uri.find('?'))
    {
        parseQuery();
        isQuery = true;
    }
    std::getline(req_stream, line);
    while (std::getline(req_stream, line) && line != "\r")
    {
        key = line.substr(0, line.find_first_of(':'));
        val = trim(line.substr(line.find_first_of(':') + 1), " \r");
        headers[key] = val;
    }
    while (std::getline(req_stream, line))
        body += line;
    return (true);
}

void HttpRequest::printRequest(void) const
{
    std::cout << "=== HTTP Request Details ===" << std::endl;
    
    // Print request line
    std::cout << "Method: " << method << std::endl;
    std::cout << "Path: " << uri << std::endl;
    std::cout << "HTTP Version: " << httpVersion << std::endl;
    std::cout << std::endl;
    
    // Print headers
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
    
    // Print body
    std::cout << "Body:" << std::endl;
    if (body.empty()) {
        std::cout << "  (no body)" << std::endl;
    } else {
        std::cout << "  Length: " << body.length() << " bytes" << std::endl;
        std::cout << "  Content: " << body << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "=========================" << std::endl;
}

// Getters implementation
const std::string& HttpRequest::getMethod() const
{
    return (method);
}

const std::string& HttpRequest::getUri() const
{
    return (uri);
}

const std::string& HttpRequest::getHttpVersion() const
{
    return (httpVersion);
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const
{
    return (headers);
}

const std::string& HttpRequest::getHeader(const std::string& key) const
{
    static const std::string empty = "";
    
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end())
        return (it->second);
    return (empty);
}
