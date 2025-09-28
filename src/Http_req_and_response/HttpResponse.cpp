/**
 * @file HttpResponse.cpp
 * @brief HTTP response generation and handling implementation
 */

#include "HttpResponse.hpp"
#include "Config.hpp"
#include "macros.hpp"

std::string HttpResponse::normalizeUri(const std::string& uri) const
{
    std::string normalized = uri;
    
    // Remove query parameters
    std::string::size_type qpos = normalized.find('?');
    if (qpos != std::string::npos) {
        normalized = normalized.substr(0, qpos);
    }
    
    // Security: remove directory traversal sequences
    size_t pos;
    while ((pos = normalized.find("..")) != std::string::npos) {
        normalized.erase(pos, 2);
    }
    
    // Remove leading slash for file path construction
    if (!normalized.empty() && normalized[0] == '/') {
        normalized = normalized.substr(1);
    }
    
    return normalized;
}

std::string HttpResponse::getContentType(const std::string& path) const
{
    std::string::size_type dot = path.rfind('.');
    if (dot == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string ext = path.substr(dot);
    
    // Common web file types
    if (ext == ".html" || ext == ".htm") return "text/html; charset=UTF-8";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".txt") return "text/plain; charset=UTF-8";
    if (ext == ".xml") return "application/xml";
    
    return "application/octet-stream";
}

std::string HttpResponse::buildResponse(const HttpRequest &request, int statusCode, 
        const std::string& contentType, const std::string& body) const
{
    std::ostringstream resp;
    
    // Status line
    resp << request.getHttpVersion() << " " << statusCode << " " 
         << getReasonPhraseFromCode(statusCode) << "\r\n";
    
    // Headers
    if (!contentType.empty()) {
        resp << "Content-Type: " << contentType << "\r\n";
    }
    if (!body.empty()) {
        resp << "Content-Length: " << body.size() << "\r\n";
    }
    resp << "Connection: close\r\n\r\n";
    
    // Body
    if (!body.empty()) {
        resp << body;
    }
    
    return resp.str();
}

std::string HttpResponse::getReasonPhraseFromCode(int statusCode)
{
    switch (statusCode) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 505: return "HTTP Version Not Supported";
        default:  return "Unknown";
    }
}

// Property setter methods
void HttpResponse::setStatusCode(int code) {
    statusCode = code;
    reasonPhrase = getReasonPhraseFromCode(code);
}

void HttpResponse::setReasonPhrase(const std::string& phrase) {
    reasonPhrase = phrase;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value) {
    headers[key] = value;
}

void HttpResponse::setBody(const std::string& body) {
    this->body = body;
    updateContentLength();
}

void HttpResponse::setContentType(const std::string& contentType) {
    headers["Content-Type"] = contentType;
}

void HttpResponse::setContentLength(size_t length) {
    std::ostringstream oss;
    oss << length;
    headers["Content-Length"] = oss.str();
}

// Property getter methods
int HttpResponse::getStatusCode() const {
    return statusCode;
}

const std::string& HttpResponse::getReasonPhrase() const {
    return reasonPhrase;
}

const std::string& HttpResponse::getHeader(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    static const std::string empty = "";
    return (it != headers.end()) ? it->second : empty;
}

const std::string& HttpResponse::getBody() const {
    return body;
}

const std::map<std::string, std::string>& HttpResponse::getHeaders() const {
    return headers;
}

void HttpResponse::updateContentLength() {
    std::ostringstream oss;
    oss << body.length();
    headers["Content-Length"] = oss.str();
}

void HttpResponse::createOkResponse(const HttpRequest &request) {
    updateContentLength();
    fullResponse.clear();
    
    if (request.getMethod() == "GET") {
        fullResponse = request.getHttpVersion() + " 200 " + getReasonPhraseFromCode(200) + "\r\n";
        fullResponse += "Content-Length: " + headers["Content-Length"] + "\r\n";
        
        if (headers.find("Content-Type") != headers.end()) {
            fullResponse += "Content-Type: " + headers["Content-Type"] + "\r\n";
        } else {
            fullResponse += "Content-Type: text/html; charset=UTF-8\r\n";
        }
        fullResponse += "\r\n";
    } else if (request.getMethod() == "DELETE") {
        fullResponse = request.getHttpVersion() + " 204 " + getReasonPhraseFromCode(204) + "\r\n";
    }
    
    fullResponse += body;
}

std::string HttpResponse::createErrorResponse(const HttpRequest &request, int errorCode) const
{
    const std::string reason = getReasonPhraseFromCode(errorCode);
    std::ostringstream body_ss;
    
    // Create simple HTML error page
    body_ss << "<html><head><title>" << errorCode << " " << reason
            << "</title></head><body>"
            << "<h1>" << errorCode << " " << reason << "</h1>"
            << "<p>The requested resource could not be processed.</p>"
            << "</body></html>";
    
    return buildResponse(request, errorCode, "text/html; charset=UTF-8", body_ss.str());
}

const std::string HttpResponse::createGetResponse(const HttpRequest &request, const ServerConfig &config)
{
    struct stat fileStat;
    std::string uri = normalizeUri(request.getUri());

    // Handle root directory - look for index files
    if (uri.empty()) {
        for (size_t i = 0; i < config.index_files.size(); ++i) {
            std::string index_path = request.getRoot() + "/" + config.index_files[i];
            if (stat(index_path.c_str(), &fileStat) == 0 && !S_ISDIR(fileStat.st_mode)) {
                uri = config.index_files[i]; 
                break;
            }
        }
        if (uri.empty()) {
            return createErrorResponse(request, HTTP_FORBIDDEN);
        }
    }
    
    std::string path = request.getRoot() + "/" + uri;
    
    // Validate file exists and is readable
    if (stat(path.c_str(), &fileStat) != 0) {
        return createErrorResponse(request, HTTP_NOT_FOUND);
    }
    if (access(path.c_str(), R_OK) != 0) {
        return createErrorResponse(request, HTTP_FORBIDDEN);
    }

    // Read file content
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return createErrorResponse(request, HTTP_INTERNAL_SERVER_ERROR);
    }

    char buffer[BUFF_SIZE];
    ssize_t bytes_read;
    body.clear();
    
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        body.append(buffer, bytes_read);
    }
    
    close(fd);
    
    if (bytes_read < 0) {
        return createErrorResponse(request, HTTP_INTERNAL_SERVER_ERROR);
    }

    // Generate response with appropriate content type
    setContentType(getContentType(path));
    createOkResponse(request);
    return fullResponse;
}

const std::string HttpResponse::createDeleteResponse(const HttpRequest &request, const ServerConfig& config) const
{
    (void)config; // Suppress unused parameter warning
    struct stat fileStat;
    
    std::string uri = normalizeUri(request.getUri());
    
    // Security: prevent deletion of important files or root
    if (uri.empty() || uri == "index.html" || uri == "login.html") {
        return createErrorResponse(request, HTTP_FORBIDDEN);
    }
    
    std::string path = request.getRoot() + "/" + uri;
    
    // Validate file exists and is not a directory
    if (stat(path.c_str(), &fileStat) != 0) {
        return createErrorResponse(request, HTTP_NOT_FOUND);
    }
    if (S_ISDIR(fileStat.st_mode)) {
        return createErrorResponse(request, HTTP_FORBIDDEN);
    }
    
    // Attempt to delete file
    if (remove(path.c_str()) != 0) {
        return createErrorResponse(request, HTTP_INTERNAL_SERVER_ERROR);
    }
    
    return buildResponse(request, 204); // No Content
}

const std::string HttpResponse::createPostResponse(const HttpRequest &request, const ServerConfig & config) const
{
    (void)config; // Suppress unused parameter warning
    return createErrorResponse(request, HTTP_NOT_IMPLEMENTED);
}

const std::string HttpResponse::createUnknowResponse(const HttpRequest &request, const ServerConfig &config) const
{
    (void)config; // Suppress unused parameter warning
    const std::string body = "<html><body><h1>405 Method Not Allowed</h1>"
                             "<p>The requested method is not supported for this resource.</p>"
                             "</body></html>";
    
    std::ostringstream resp;
    resp << request.getHttpVersion() << " 405 Method Not Allowed\r\n"
         << "Allow: GET, POST, DELETE\r\n"
         << "Content-Type: text/html; charset=UTF-8\r\n"
         << "Content-Length: " << body.size() << "\r\n"
         << "Connection: close\r\n\r\n"
         << body;
         
    return resp.str();
}

std::string HttpResponse::createResponse(const HttpRequest &request, const ServerConfig& config)
{
    HttpResponse response;
    const std::string& method = request.getMethod();

    if (method == "GET") {
        return response.createGetResponse(request, config);
    } else if (method == "POST") {
        return response.createPostResponse(request, config);
    } else if (method == "DELETE") {
        return response.createDeleteResponse(request, config);
    } else {
        return response.createUnknowResponse(request, config);
    }
}