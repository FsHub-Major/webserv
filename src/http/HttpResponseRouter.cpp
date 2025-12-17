#include "HttpResponse.hpp"

#include <sstream>

// Define missing function: 405 for unknown/unsupported methods
const std::string HttpResponse::createUnknowResponse(const HttpRequest &request, const ServerConfig &config) const
{
    (void)config;

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

// Entry point: routes to method-specific handler
std::string HttpResponse::createResponse(const HttpRequest &request, const ServerConfig &config)
{
    HttpResponse response;
    const std::string &method = request.getMethod();

    if (method == "GET")
        return response.createGetResponse(request, config);
    if (method == "POST")
        return response.createPostResponse(request, config);
    if (method == "DELETE")
        return response.createDeleteResponse(request, config);

    return response.createUnknowResponse(request, config);
}
