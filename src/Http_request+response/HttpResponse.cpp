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
    // Build proper HTTP/1.1 response with CRLF and headers
    fullResponse = request.getHttpVersion() + " 200 " + getReasonPhraseFromCode(200) + "\r\n";
    // Content-Length is required for persistent connections
    fullResponse += "Content-Length: " + headers["Content-Length"] + "\r\n";
    // Add Content-Type if provided
    if (headers.find("Content-Type") != headers.end())
        fullResponse += "Content-Type: " + headers["Content-Type"] + "\r\n";
    else
        fullResponse += "Content-Type: text/plain; charset=UTF-8\r\n";
    // Close by default for simplicity
    fullResponse += "Connection: close\r\n";
    // End of headers
    fullResponse += "\r\n";
    fullResponse += body;
}

const std::string HttpResponse::createPostResponse(const HttpRequest &request) const
{
    (void)request; // suppress unused parameter for now
    // Minimal 501 Not Implemented response for POST until implemented
    std::ostringstream oss;
    const std::string reason = getReasonPhraseFromCode(HTTP_NOT_IMPLEMENTED);
    oss << "HTTP/1.1 " << HTTP_NOT_IMPLEMENTED << ' ' << reason << "\r\n";
    const std::string msg = "POST not implemented";
    oss << "Content-Type: text/plain; charset=UTF-8\r\n";
    oss << "Content-Length: " << msg.size() << "\r\n";
    oss << "Connection: close\r\n\r\n";
    oss << msg;
    return oss.str();
}

const std::string HttpResponse::createDeleteResponse(const HttpRequest &request) const
{
    (void)request; // suppress unused parameter for now
    // Minimal 501 Not Implemented response for DELETE until implemented
    std::ostringstream oss;
    const std::string reason = getReasonPhraseFromCode(HTTP_NOT_IMPLEMENTED);
    oss << "HTTP/1.1 " << HTTP_NOT_IMPLEMENTED << ' ' << reason << "\r\n";
    const std::string msg = "DELETE not implemented";
    oss << "Content-Type: text/plain; charset=UTF-8\r\n";
    oss << "Content-Length: " << msg.size() << "\r\n";
    oss << "Connection: close\r\n\r\n";
    oss << msg;
    return oss.str();
}


const std::string HttpResponse::createGetResponse(const HttpRequest &request)
{
    struct stat fileStat;
    int fd;
    std::string path;
    char buffer[BUFF_SIZE];
    ssize_t bytes_read = 0;

    // normalize path and strip query string if present
    std::string uri = request.getUri();
    std::string::size_type qpos = uri.find('?');
    if (qpos != std::string::npos)
        uri = uri.substr(0, qpos);

    path = request.getRoot() + uri;
    // ensure relative path by prepending '.'
    path.insert(path.begin(), '.');

    if (stat(path.c_str(), &fileStat) == 0)
    {
        if (access(path.c_str(), R_OK) == 0)
        {
            fd = open(path.c_str(), O_RDONLY);
            if (fd >= 0)
            {
                body.clear();
                while ((bytes_read = read(fd, buffer, BUFF_SIZE)) > 0)
                    body.append(buffer, bytes_read);
                if (bytes_read < 0)
                {
                    close(fd);
                    std::ostringstream oss;
                    const std::string reason = getReasonPhraseFromCode(HTTP_INTERNAL_SERVER_ERROR);
                    const std::string msg = "Read error";
                    oss << request.getHttpVersion() << ' ' << HTTP_INTERNAL_SERVER_ERROR << ' ' << reason << "\r\n";
                    oss << "Content-Type: text/plain; charset=UTF-8\r\n";
                    oss << "Content-Length: " << msg.size() << "\r\n";
                    oss << "Connection: close\r\n\r\n";
                    oss << msg;
                    return oss.str();
                }
                close(fd);
                // set a default content type if not already provided
                if (headers.find("Content-Type") == headers.end())
                    headers["Content-Type"] = "text/html; charset=UTF-8";
                createOkResponse(request);
                return fullResponse;
            }
            // open failed
            std::ostringstream oss;
            const std::string reason = getReasonPhraseFromCode(HTTP_INTERNAL_SERVER_ERROR);
            const std::string msg = "Open error";
            oss << request.getHttpVersion() << ' ' << HTTP_INTERNAL_SERVER_ERROR << ' ' << reason << "\r\n";
            oss << "Content-Type: text/plain; charset=UTF-8\r\n";
            oss << "Content-Length: " << msg.size() << "\r\n";
            oss << "Connection: close\r\n\r\n";
            oss << msg;
            return oss.str();
        }
        // Forbidden
        std::ostringstream oss;
        const std::string reason = getReasonPhraseFromCode(HTTP_FORBIDDEN);
        const std::string msg = "Forbidden";
        oss << request.getHttpVersion() << ' ' << HTTP_FORBIDDEN << ' ' << reason << "\r\n";
        oss << "Content-Type: text/plain; charset=UTF-8\r\n";
        oss << "Content-Length: " << msg.size() << "\r\n";
        oss << "Connection: close\r\n\r\n";
        oss << msg;
        return oss.str();
    }
    // Not found
    {
        std::ostringstream oss;
        const std::string reason = getReasonPhraseFromCode(HTTP_NOT_FOUND);
        const std::string msg = "Not Found";
        oss << request.getHttpVersion() << ' ' << HTTP_NOT_FOUND << ' ' << reason << "\r\n";
        oss << "Content-Type: text/plain; charset=UTF-8\r\n";
        oss << "Content-Length: " << msg.size() << "\r\n";
        oss << "Connection: close\r\n\r\n";
        oss << msg;
        return oss.str();
    }
}

// Implement the declared but undefined helpers to avoid linker errors
void HttpResponse::createErrorResponse(const HttpRequest &request, int errorCode) const
{
    (void)request;
    (void)errorCode;
    // Intentionally left minimal; createGetResponse builds and returns error strings directly.
}

std::string HttpResponse::createResponse(const HttpRequest &request)
{
    HttpResponse response;
    std::string rawResponse;

    if (request.getMethod() == "GET")
        rawResponse = response.createGetResponse(request);
    else if (request.getMethod() == "POST")
        rawResponse = response.createPostResponse(request);
    else if (request.getMethod() == "DELETE")
        rawResponse = response.createDeleteResponse(request);
    else
        rawResponse = response.createUnknowResponse(request);
    return (rawResponse);
}

// Provide a minimal implementation for unknown methods to satisfy linker
const std::string HttpResponse::createUnknowResponse(const HttpRequest &request) const
{
    std::ostringstream oss;
    const std::string reason = getReasonPhraseFromCode(HTTP_METHOD_NOT_ALLOWED);
    const std::string msg = "Method Not Allowed";
    oss << request.getHttpVersion() << ' ' << HTTP_METHOD_NOT_ALLOWED << ' ' << reason << "\r\n";
    oss << "Allow: GET, POST, DELETE\r\n";
    oss << "Content-Type: text/plain; charset=UTF-8\r\n";
    oss << "Content-Length: " << msg.size() << "\r\n";
    oss << "Connection: close\r\n\r\n";
    oss << msg;
    return oss.str();
}