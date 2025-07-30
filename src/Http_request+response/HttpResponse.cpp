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
    fullResponse = request.getHttpVersion() + " " + "200 " + getReasonPhraseFromCode(200);
    fullResponse =fullResponse  + "/n" + "Content-Length" + headers["Content-Length"];
    fullResponse += "\n\n";
    fullResponse += body;
}

const std::string HttpResponse::createPostResponse(const HttpRequest &request) const
{

}

const std::string HttpResponse::createDeleteResponse(const HttpRequest &request) const
{

}


const std::string HttpResponse::createGetResponse(const HttpRequest &request)
{
    struct stat fileStat;
    int fd;
    std::string path;
    char *buffer;

    if (!request.getUri().find('?'))
    {
        path = request.getRoot() + request.getUri();
        path.insert(path.begin(), '.');
        if (stat(path.c_str(), &fileStat) == 0)
        {
            if (access(path.c_str(), R_OK) == 0)
            {
                fd = open(path.c_str(), O_RDONLY);
                if (fd > 0)
                {
                    while (read(fd, buffer, BUFF_SIZE) > 0)
                        body += buffer;
                    createOkResponse(request);
                    close(fd);
                }
            }
            else
                createErrorResponse(request, HTTP_FORBIDDEN);
        }
        else
            createErrorResponse(request, HTTP_NOT_FOUND);
    }

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