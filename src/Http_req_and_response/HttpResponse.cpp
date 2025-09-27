#include "HttpResponse.hpp"
#include "Config.hpp"
#include "macros.hpp"

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
    return statusCode;
}

const std::string& HttpResponse::getReasonPhrase() const
{
    return reasonPhrase;
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
    return body;
}

const std::map<std::string, std::string>& HttpResponse::getHeaders() const
{
    return headers;
}

void HttpResponse::updateContentLength()
{
    std::ostringstream oss;
    oss << body.length();
    headers["Content-Length"] = oss.str();
}

void HttpResponse::createOkResponse(const HttpRequest &request)
{
    updateContentLength();
    // Proper HTTP/1.1 formatting using CRLF
    fullResponse.clear();
    fullResponse = request.getHttpVersion() + " 200 " + getReasonPhraseFromCode(200) + "\r\n";
    fullResponse += "Content-Length: " + headers["Content-Length"] + "\r\n";
    if (headers.find("Content-Type") != headers.end())
        fullResponse += "Content-Type: " + headers["Content-Type"] + "\r\n";
    else
        fullResponse += "Content-Type: text/html; charset=UTF-8\r\n";
    fullResponse += "Connection: close\r\n";
    fullResponse += "\r\n";
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
    const std::string errBody = body_ss.str();

    std::ostringstream resp;
    resp << request.getHttpVersion() << " " << errorCode << " " << reason << "\r\n";
    resp << "Content-Type: text/html; charset=UTF-8\r\n";
    resp << "Content-Length: " << errBody.size() << "\r\n";
    resp << "Connection: close\r\n";
    resp << "\r\n";
    resp << errBody;
    return resp.str();
}

const std::string HttpResponse::createPostResponse(const HttpRequest &request, const ServerConfig & config) const
{

    (void) config;
    // Minimal 501 (adjust later when implementing POST)
    const std::string body = "<html><body><h1>501 Not Implemented</h1></body></html>";
    std::ostringstream resp;
    resp << request.getHttpVersion() << " 501 Not Implemented\r\n";
    resp << "Content-Type: text/html; charset=UTF-8\r\n";
    resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Connection: close\r\n";
    resp << "\r\n";
    resp << body;
    return resp.str();
}


const std::string HttpResponse::createDeleteResponse(const HttpRequest &request, const ServerConfig &config) const
{
    (void) config;
    // Minimal 501 (adjust later when implementing DELETE)
    const std::string body = "<html><body><h1>501 Not Implemented</h1></body></html>";
    std::ostringstream resp;
    resp << request.getHttpVersion() << " 501 Not Implemented\r\n";
    resp << "Content-Type: text/html; charset=UTF-8\r\n";
    resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Connection: close\r\n";
    resp << "\r\n";
    resp << body;
    return resp.str();
}

const std::string HttpResponse::createGetResponse(const HttpRequest &request, const ServerConfig &config)
{
    struct stat fileStat;
    int fd;
    std::string path;
    char buffer[BUFF_SIZE];
    ssize_t n;

    // Build filesystem path and strip query if present
    std::string uri = request.getUri();


    std::cout << "URI => " << uri << std::endl;

    std::string::size_type qpos = uri.find('?');
    if (qpos != std::string::npos)
        uri = uri.substr(0, qpos);

    // remove dangerous characters and normalize path 
    // remove .. and / 
    size_t pos;
    while ((pos = uri.find("..")) != std::string::npos)
        uri.erase(pos, 2);
    
    if (!uri.empty() && uri[0] == '/')
        uri = uri.substr(1);

    // root dir request, check for index files
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
            return (createErrorResponse(request, HTTP_FORBIDDEN));
    }
    
    path = request.getRoot() + "/" + uri;

    std::cout << "PATH => " << path << std::endl;

    if (stat(path.c_str(), &fileStat) != 0) {
        return createErrorResponse(request, HTTP_NOT_FOUND);
    }
    if (access(path.c_str(), R_OK) != 0) {
        return createErrorResponse(request, HTTP_FORBIDDEN);
    }

    fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return createErrorResponse(request, HTTP_INTERNAL_SERVER_ERROR);
    }

    body.clear();
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        body.append(buffer, n);
    }
    if (n < 0) {
        close(fd);
        return createErrorResponse(request, HTTP_INTERNAL_SERVER_ERROR);
    }
    close(fd);

    // Very basic content type guessing
    std::string ctype = "application/octet-stream";
    std::string::size_type dot = path.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = path.substr(dot);
        if (ext == ".html" || ext == ".htm") ctype = "text/html; charset=UTF-8";
        else if (ext == ".css") ctype = "text/css";
        else if (ext == ".js") ctype = "application/javascript";
        else if (ext == ".json") ctype = "application/json";
        else if (ext == ".png") ctype = "image/png";
        else if (ext == ".jpg" || ext == ".jpeg") ctype = "image/jpeg";
        else if (ext == ".gif") ctype = "image/gif";
    }
    setContentType(ctype);

    createOkResponse(request);
    return fullResponse;
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
    resp << "Connection: close\r\n";
    resp << "\r\n";
    resp << body;
    return resp.str();
}

std::string HttpResponse::createResponse(const HttpRequest &request, const ServerConfig& config)
{
    (void) config;
    HttpResponse response;
    std::string rawResponse;

    if (request.getMethod() == "GET")
        rawResponse = response.createGetResponse(request, config);
    else if (request.getMethod() == "POST")
        rawResponse = response.createPostResponse(request, config);
    else if (request.getMethod() == "DELETE")
        rawResponse = response.createDeleteResponse(request, config);
    else
        rawResponse = response.createUnknowResponse(request, config);
    return (rawResponse);
}