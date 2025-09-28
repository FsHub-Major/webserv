#include "HttpResponse.hpp"
#include "Config.hpp"
#include "macros.hpp"

// Common helper to normalize and sanitize URI paths
std::string HttpResponse::normalizeUri(const std::string& uri)
{
    std::string normalized;
    
    normalized = uri;
    // Remove query parameters
    std::string::size_type qpos = normalized.find('?');
    if (qpos != std::string::npos)
        normalized = normalized.substr(0, qpos);
    
    // Security: remove .. sequences
    size_t pos;
    while ((pos = normalized.find("..")) != std::string::npos)
        normalized.erase(pos, 2);
    
    // Remove leading slash
    if (!normalized.empty() && normalized[0] == '/')
        normalized = normalized.substr(1);
    
    return normalized;
}

// Common helper to determine content type from file extension
std::string HttpResponse::getContentType(const std::string& path)
{
    std::string::size_type dot = path.rfind('.');
    if (dot == std::string::npos)
        return "application/octet-stream";
    
    std::string ext = path.substr(dot);
    if (ext == ".html" || ext == ".htm") return "text/html; charset=UTF-8";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    return "application/octet-stream";
}

// Common helper to build HTTP response with headers
std::string HttpResponse::buildResponse(const HttpRequest &request, int statusCode, 
        const std::string& contentType, const std::string& body)
{
    std::ostringstream resp;
    resp << request.getHttpVersion() << " " << statusCode << " " 
         << HttpResponse::getReasonPhraseFromCode(statusCode) << "\r\n";
    
    if (!contentType.empty())
        resp << "Content-Type: " << contentType << "\r\n";
    if (!body.empty())
        resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Connection: close\r\n\r\n";
    
    if (!body.empty())
        resp << body;
    
    return resp.str();
}

// Helper method to get reason phrase from status code
std::string HttpResponse::getReasonPhraseFromCode(int statusCode)
{
    switch (statusCode) {
        case 200: return ("OK");
        case 201: return ("Created");
        case 204: return ("No Content");
        case 301: return ("Moved Permanently");
        case 302: return ("Found");
        case 304: return ("Not Modified");
        case 400: return ("Bad Request");
        case 401: return ("Unauthorized");
        case 403: return ("Forbidden");
        case 404: return ("Not Found");
        case 405: return ("Method Not Allowed");
        case 408: return ("Request Timeout");
        case 413: return ("Payload Too Large");
        case 414: return ("URI Too Long");
        case 500: return ("Internal Server Error");
        case 501: return ("Not Implemented");
        case 502: return ("Bad Gateway");
        case 503: return ("Service Unavailable");
        case 505: return ("HTTP Version Not Supported");
        default:  return ("Unknown");
    }
}

// Setters
void HttpResponse::setStatusCode(int code)
{
    statusCode = code;
    reasonPhrase = getReasonPhraseFromCode(code);
}

void HttpResponse::setReasonPhrase(const std::string& phrase)
{
    reasonPhrase = phrase;
}

void HttpResponse::setHeader(const std::string& key, const std::string& value)
{
    headers[key] = value;
}

void HttpResponse::setBody(const std::string& body)
{
    this->body = body;
    updateContentLength();
}

void HttpResponse::setContentType(const std::string& contentType)
{
    headers["Content-Type"] = contentType;
}

void HttpResponse::setContentLength(size_t length)
{
    std::ostringstream oss;
    oss << length;
    headers["Content-Length"] = oss.str();
}

// Getters
int HttpResponse::getStatusCode() const
{
    return (statusCode);
}

const std::string& HttpResponse::getReasonPhrase() const
{
    return (reasonPhrase);
}

const std::string& HttpResponse::getHeader(const std::string& key) const
{
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end()) {
        return it->second;
    }
    static const std::string empty = "";
    return empty;
}

const std::string& HttpResponse::getBody() const
{
    return (body);
}

const std::map<std::string, std::string>& HttpResponse::getHeaders() const
{
    return (headers);
}

void HttpResponse::updateContentLength()
{
    std::ostringstream oss;
    oss << body.length();
    headers["Content-Length"] = oss.str();
}

void HttpResponse::createOkResponse(const HttpRequest &request)
{
    if (request.getMethod() == "GET")
    {
        updateContentLength();
        fullResponse.clear();
        fullResponse = request.getHttpVersion() + " 200 " + getReasonPhraseFromCode(200) + "\r\n";
        fullResponse += "Content-Length: " + headers["Content-Length"] + "\r\n";
        if (headers.find("Content-Type") != headers.end())
            fullResponse += "Content-Type: " + headers["Content-Type"] + "\r\n";
        else
            fullResponse += "Content-Type: text/html; charset=UTF-8\r\n";
        fullResponse += "\r\n";
    }
    else if (request.getMethod() == "DELETE")
    {
        fullResponse = request.getHttpVersion() + " 204 " + getReasonPhraseFromCode(204) + "\r\n";
    }
    fullResponse += body;
}

// Define missing function: builds an error response string
std::string HttpResponse::createErrorResponse(const HttpRequest &request, int errorCode) const
{
    const std::string reason = getReasonPhraseFromCode(errorCode);
    std::ostringstream body_ss;
    body_ss << "<html><head><title>" << errorCode << " " << reason
            << "</title></head><body><h1>" << errorCode << " " << reason
            << "</h1></body></html>";
    
    return (buildResponse(request, errorCode, "text/html; charset=UTF-8", body_ss.str()));
}

const std::string HttpResponse::createPostResponse(const HttpRequest &request, const ServerConfig & config) const
{
    (void)config;
    return (createErrorResponse(request, HTTP_NOT_IMPLEMENTED));
}

const std::string HttpResponse::createGetResponse(const HttpRequest &request, const ServerConfig &config)
{
    struct stat fileStat;
    char buffer[BUFF_SIZE];
    std::string uri;
    ssize_t n;

    uri = normalizeUri(request.getUri());

    // Handle root directory - check for index files
    if (uri.empty())
    {
        for (size_t i = 0; i < config.index_files.size(); ++i)
        {
            std::string index_path = request.getRoot() + "/" + config.index_files[i];
            if (stat(index_path.c_str(), &fileStat) == 0 && !S_ISDIR(fileStat.st_mode))
            {
                uri = config.index_files[i]; 
                break;
            }
        }
        if (uri.empty())
            return createErrorResponse(request, HTTP_FORBIDDEN);
    }
    
    std::string path = request.getRoot() + "/" + uri;
    // File validation
    if (stat(path.c_str(), &fileStat) != 0)
        return createErrorResponse(request, HTTP_NOT_FOUND);
    if (access(path.c_str(), R_OK) != 0)
        return createErrorResponse(request, HTTP_FORBIDDEN);

    // Read file
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return (createErrorResponse(request, HTTP_INTERNAL_SERVER_ERROR));

    body.clear();
    while ((n = read(fd, buffer, sizeof(buffer))) > 0)
        body.append(buffer, n);
    
    close(fd);
    
    if (n < 0)
        return createErrorResponse(request, HTTP_INTERNAL_SERVER_ERROR);

    // Set content type and create response
    setContentType(getContentType(path));
    createOkResponse(request);
    return (fullResponse);
}

const std::string HttpResponse::createDeleteResponse(const HttpRequest &request, const ServerConfig& config) const
{
    (void)config;
    struct stat fileStat;
    
    std::string uri = normalizeUri(request.getUri());
    
    // Prevent deletion of important files or empty paths
    if (uri.empty() || uri == "index.html" || uri == "login.html")
        return createErrorResponse(request, HTTP_FORBIDDEN);
    
    std::string path = request.getRoot() + "/" + uri;
    
    // File validation
    if (stat(path.c_str(), &fileStat) != 0)
        return createErrorResponse(request, HTTP_NOT_FOUND);
    if (S_ISDIR(fileStat.st_mode))
        return createErrorResponse(request, HTTP_FORBIDDEN);
    // Delete file
    if (remove(path.c_str()) != 0)
        return createErrorResponse(request, HTTP_INTERNAL_SERVER_ERROR);
    return (buildResponse(request, 204));
}

// Define missing function: 405 for unknown/unsupported methods
const std::string HttpResponse::createUnknowResponse(const HttpRequest &request, const ServerConfig &config) const
{
    (void) config;
    const std::string body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
    
    std::ostringstream resp;
    resp << request.getHttpVersion() << " 405 Method Not Allowed\r\n";
    resp << "Allow: GET, POST, DELETE\r\n";
    resp << "Content-Type: text/html; charset=UTF-8\r\n";
    resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Connection: close\r\n\r\n";
    resp << body;
    return resp.str();
}

std::string HttpResponse::createResponse(const HttpRequest &request, const ServerConfig& config)
{
    HttpResponse response;
    const std::string& method = request.getMethod();

    if (method == "GET")
        return response.createGetResponse(request, config);
    else if (method == "POST")
        return response.createPostResponse(request, config);
    else if (method == "DELETE")
        return response.createDeleteResponse(request, config);
    else
        return response.createUnknowResponse(request, config);
}