#include "HttpResponse.hpp"

#include "HttpResponseHelpers.hpp"

#include <sstream>

// Helper: map status code to reason phrase
std::string HttpResponse::getReasonPhraseFromCode(int statusCode)
{
    switch (statusCode)
    {
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
        default: return ("Unknown");
    }
}

// Setters
void HttpResponse::setStatusCode(int code)
{
    statusCode = code;
    reasonPhrase = getReasonPhraseFromCode(code);
}

void HttpResponse::setReasonPhrase(const std::string &phrase)
{
    reasonPhrase = phrase;
}

void HttpResponse::setHeader(const std::string &key, const std::string &value)
{
    headers[key] = value;
}

void HttpResponse::setBody(const std::string &newBody)
{
    body = newBody;
    updateContentLength();
}

void HttpResponse::setContentType(const std::string &contentType)
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

const std::string &HttpResponse::getReasonPhrase() const
{
    return reasonPhrase;
}

const std::string &HttpResponse::getHeader(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it = headers.find(key);

    if (it != headers.end())
        return it->second;

    static const std::string empty = "";
    return empty;
}

const std::string &HttpResponse::getBody() const
{
    return body;
}

const std::map<std::string, std::string> &HttpResponse::getHeaders() const
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

    fullResponse.clear();
    fullResponse = request.getHttpVersion() + " 200 "
        + getReasonPhraseFromCode(200) + "\r\n";
    fullResponse += "Content-Length: " + headers["Content-Length"] + "\r\n";

    if (headers.find("Content-Type") != headers.end())
        fullResponse += "Content-Type: " + headers["Content-Type"] + "\r\n";
    else
        fullResponse += "Content-Type: text/html; charset=UTF-8\r\n";

    fullResponse += "Connection: close\r\n\r\n";
    fullResponse += body;
}

// Define missing function: builds an error response string
std::string HttpResponse::createErrorResponse(const HttpRequest &request, int errorCode) const
{
    const std::string reason = getReasonPhraseFromCode(errorCode);

    std::ostringstream bodyStream;
    bodyStream << "<html><head><title>" << errorCode << " " << reason
        << "</title></head><body><h1>" << errorCode << " " << reason
        << "</h1></body></html>";

    const std::string errBody = bodyStream.str();

    std::ostringstream resp;
    resp << request.getHttpVersion() << " " << errorCode << " " << reason << "\r\n";
    resp << "Content-Type: text/html; charset=UTF-8\r\n";
    resp << "Content-Length: " << errBody.size() << "\r\n";
    resp << "Connection: close\r\n\r\n";
    resp << errBody;

    return resp.str();
}
