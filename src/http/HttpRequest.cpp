#include "HttpRequest.hpp"
#include "ext_libs.hpp"

HttpRequest::HttpRequest(void)
{
    isQuery = false;
}

void HttpRequest::parseQuery(void)
{
    vector<std::string> query;
    std::string key, val;

    query = split(uri.substr(uri.find_first_of('?') + 1), "&");
    for(std::size_t i = 0; i < query.size(); i++)
    {
        std::size_t eq = query[i].find_first_of('=');
        if (eq == std::string::npos)
        {
            key = query[i];
            val.clear();
        }
        else
        {
            key = query[i].substr(0, eq);
            val = query[i].substr(eq + 1);
        }
        queryParams[key] = val;
    }
}

bool HttpRequest::parseRequest(const std::string &request, const std::string root)
{
    std::string line;
    std::string key;
    std::string val;

    rawRequest = request;
    this->root = root;
    std::istringstream req_stream(request);
    req_stream >> method >> uri >> httpVersion;

    if (uri.find('?') != std::string::npos)
    {
        parseQuery();
        isQuery = true;
    }
    std::getline(req_stream, line);
    while (std::getline(req_stream, line) && line != "\r")
    {
        size_t colon_pos = line.find_first_of(':');
        if (colon_pos == std::string::npos || colon_pos + 1 >= line.size())
            continue;
        key = line.substr(0, colon_pos);
        val = trim(line.substr(colon_pos + 1), " \r");
        headers[key] = val;
    }
    
    std::string::size_type header_end = request.find("\r\n\r\n");
    if (header_end != std::string::npos)
        body = request.substr(header_end + 4);
    else
        body.clear();
    return (true);
}

void HttpRequest::printRequest(void) const
{
    std::cout << "=== HTTP Request Details ===" << std::endl;
    std::cout << "Method: " << method << std::endl;
    std::cout << "Path: " << uri << std::endl;
    std::cout << "HTTP Version: " << httpVersion << std::endl;
    std::cout << std::endl;
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

const std::string& HttpRequest::getMethod() const { return method; }
const std::string& HttpRequest::getUri() const { return uri; }
const std::string& HttpRequest::getHttpVersion() const { return httpVersion; }
const std::map<std::string, std::string>& HttpRequest::getHeaders() const { return headers; }
const std::string& HttpRequest::getHeader(const std::string& key) const {
    static const std::string empty = "";
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end()) return it->second;
    return empty;
}
const std::string &HttpRequest::getRoot() const { return root; }
const std::string &HttpRequest::getBody() const { return body; }
