#include "HttpResponse.hpp"

#include "FastCgiClient.hpp"
#include "HttpResponseHelpers.hpp"
#include "macros.hpp"

#include <sstream>

namespace {

const LocationConfig *matchBestLocation(const ServerConfig &config, const std::string &uri, size_t &bestLen)
{
    const LocationConfig *best = NULL;

    bestLen = 0;
    for (size_t i = 0; i < config.locations.size(); ++i)
    {
        const std::string &p = config.locations[i].location;
        if (!p.empty() && uri.compare(0, p.size(), p) == 0 && p.size() > bestLen)
        {
            best = &config.locations[i];
            bestLen = p.size();
        }
    }
    return best;
}

bool isMethodAllowed(const std::vector<std::string> &methods, const std::string &method)
{
    for (size_t i = 0; i < methods.size(); ++i)
    {
        if (methods[i] == method)
            return true;
    }
    return false;
}

std::string make405(const HttpRequest &request, const std::vector<std::string> &allow, const std::string &extra)
{
    const std::string msg = extra.empty()
        ? "<html><body><h1>405 Method Not Allowed</h1></body></html>"
        : extra;

    std::ostringstream resp;
    resp << request.getHttpVersion() << " 405 Method Not Allowed\r\n";
    if (!allow.empty())
        resp << http_response_helpers::buildAllowHeader(allow);
    else
        resp << "Allow: GET\r\n";
    resp << "Content-Type: text/html; charset=UTF-8\r\n";
    resp << "Content-Length: " << msg.size() << "\r\n";
    resp << "Connection: close\r\n\r\n";
    resp << msg;

    return resp.str();
}

unsigned long parseContentLengthOrZero(const std::string &value)
{
    std::string cl = value;

    while (!cl.empty() && (cl[0] == ' ' || cl[0] == '\t'))
        cl.erase(0, 1);
    while (!cl.empty() && (cl[cl.size() - 1] == ' ' || cl[cl.size() - 1] == '\t'))
        cl.erase(cl.size() - 1, 1);

    for (size_t i = 0; i < cl.size(); ++i)
    {
        if (cl[i] < '0' || cl[i] > '9')
            return 0;
    }

    unsigned long out = 0;
    std::istringstream iss(cl);
    iss >> out;
    return out;
}

} // namespace

const std::string HttpResponse::createPostResponse(const HttpRequest &request, const ServerConfig &config) const
{
    std::string uri = http_response_helpers::stripQuery(request.getUri());

    size_t bestLen = 0;
    const LocationConfig *best = matchBestLocation(config, uri, bestLen);

    if (!best)
        return make405(request, std::vector<std::string>(),
            "<html><body><h1>405 Method Not Allowed</h1><p>No matching location</p></body></html>");

    if (!isMethodAllowed(best->allowed_methods, "POST"))
        return make405(request, best->allowed_methods, "");

    const std::map<std::string, std::string> &hdrs = request.getHeaders();
    std::map<std::string, std::string>::const_iterator it = hdrs.find("Content-Length");

    if (it == hdrs.end())
        it = hdrs.find("content-length");

    if (it == hdrs.end())
    {
        const std::string msg = "<html><body><h1>411 Length Required</h1></body></html>";
        std::ostringstream resp;
        resp << request.getHttpVersion() << " 411 Length Required\r\n";
        resp << "Content-Type: text/html; charset=UTF-8\r\n";
        resp << "Content-Length: " << msg.size() << "\r\n";
        resp << "Connection: close\r\n\r\n";
        resp << msg;
        return resp.str();
    }

    const unsigned long contentLength = parseContentLengthOrZero(it->second);
    const std::string &bodyRef = request.getBody();

    if (config.client_max_body_size > 0
        && (contentLength > config.client_max_body_size || bodyRef.size() > config.client_max_body_size))
    {
        const std::string msg = "<html><body><h1>413 Payload Too Large</h1></body></html>";
        std::ostringstream resp;
        resp << request.getHttpVersion() << " 413 Payload Too Large\r\n";
        resp << "Content-Type: text/html; charset=UTF-8\r\n";
        resp << "Content-Length: " << msg.size() << "\r\n";
        resp << "Connection: close\r\n\r\n";
        resp << msg;
        return resp.str();
    }

    if (bodyRef.size() < contentLength)
    {
        const std::string msg = "<html><body><h1>400 Bad Request</h1><p>Incomplete body</p></body></html>";
        std::ostringstream resp;
        resp << request.getHttpVersion() << " 400 Bad Request\r\n";
        resp << "Content-Type: text/html; charset=UTF-8\r\n";
        resp << "Content-Length: " << msg.size() << "\r\n";
        resp << "Connection: close\r\n\r\n";
        resp << msg;
        return resp.str();
    }

    std::string baseDir;
    if (!best->upload_dir.empty())
        baseDir = best->upload_dir;
    else if (!best->path.empty())
        baseDir = best->path;
    else if (best->location == "/")
        baseDir = request.getRoot();
    else
    {
        std::string loc = best->location;
        if (!loc.empty() && loc[0] == '/')
            loc.erase(0, 1);
        baseDir = request.getRoot() + "/" + loc;
    }

    std::string suffix = uri.substr(bestLen);
    if (!suffix.empty() && suffix[0] == '/')
        suffix.erase(0, 1);

    if (suffix.empty() || suffix.find("..") != std::string::npos)
    {
        const std::string msg = "<html><body><h1>400 Bad Request</h1><p>Invalid target path</p></body></html>";
        std::ostringstream resp;
        resp << request.getHttpVersion() << " 400 Bad Request\r\n";
        resp << "Content-Type: text/html; charset=UTF-8\r\n";
        resp << "Content-Length: " << msg.size() << "\r\n";
        resp << "Connection: close\r\n\r\n";
        resp << msg;
        return resp.str();
    }

    std::string targetPath = baseDir;
    if (!targetPath.empty() && targetPath[targetPath.size() - 1] != '/')
        targetPath += "/";
    targetPath += suffix;

    if (http_response_helpers::isFastCgiRequest(best, targetPath))
    {
        struct stat st;
        if (stat(targetPath.c_str(), &st) != 0)
            return createErrorResponse(request, HTTP_NOT_FOUND);
        if (access(targetPath.c_str(), R_OK) != 0)
            return createErrorResponse(request, HTTP_FORBIDDEN);

        FastCgiClient fcgi(request, config, *best, targetPath);
        return fcgi.execute();
    }

    const int wfd = open(targetPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (wfd < 0)
    {
        const std::string msg = "<html><body><h1>500 Internal Server Error</h1><p>Cannot open target</p></body></html>";
        std::ostringstream resp;
        resp << request.getHttpVersion() << " 500 Internal Server Error\r\n";
        resp << "Content-Type: text/html; charset=UTF-8\r\n";
        resp << "Content-Length: " << msg.size() << "\r\n";
        resp << "Connection: close\r\n\r\n";
        resp << msg;
        return resp.str();
    }

    const ssize_t toWrite = static_cast<ssize_t>(contentLength);
    ssize_t written = 0;
    const char *data = bodyRef.data();

    while (written < toWrite)
    {
        const ssize_t n = write(wfd, data + written, toWrite - written);
        if (n < 0)
        {
            close(wfd);
            remove(targetPath.c_str());

            const std::string msg = "<html><body><h1>500 Internal Server Error</h1><p>Write failed</p></body></html>";
            std::ostringstream resp;
            resp << request.getHttpVersion() << " 500 Internal Server Error\r\n";
            resp << "Content-Type: text/html; charset=UTF-8\r\n";
            resp << "Content-Length: " << msg.size() << "\r\n";
            resp << "Connection: close\r\n\r\n";
            resp << msg;
            return resp.str();
        }
        written += n;
    }

    close(wfd);

    const std::string okBody = "<html><body><h1>201 Created</h1></body></html>";

    std::ostringstream resp;
    resp << request.getHttpVersion() << " 201 Created\r\n";
    resp << "Location: " << uri << "\r\n";
    resp << "Content-Type: text/html; charset=UTF-8\r\n";
    resp << "Content-Length: " << okBody.size() << "\r\n";
    resp << "Connection: close\r\n\r\n";
    resp << okBody;

    return resp.str();
}
