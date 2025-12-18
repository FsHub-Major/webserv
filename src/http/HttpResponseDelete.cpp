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

std::string make405(const HttpRequest &request, const std::vector<std::string> &allow)
{
    const std::string msg = "<html><body><h1>405 Method Not Allowed</h1></body></html>";

    std::ostringstream resp;
    resp << request.getHttpVersion() << " 405 Method Not Allowed\r\n";
    if (!allow.empty())
        resp << http_response_helpers::buildAllowHeader(allow);
    resp << "Content-Type: text/html; charset=UTF-8\r\n";
    resp << "Content-Length: " << msg.size() << "\r\n";
    resp << "Connection: close\r\n\r\n";
    resp << msg;

    return resp.str();
}

} // namespace

const std::string HttpResponse::createDeleteResponse(const HttpRequest &request, const ServerConfig &config) const
{
    std::string uri = http_response_helpers::stripQuery(request.getUri());

    size_t bestLen = 0;
    const LocationConfig *best = matchBestLocation(config, uri, bestLen);

    std::string baseDir;
    std::string suffix;

    if (best)
    {
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

        suffix = uri.substr(bestLen);
        if (!suffix.empty() && suffix[0] == '/')
            suffix.erase(0, 1);
    }
    else
    {
        baseDir = request.getRoot();
        if (!uri.empty() && uri[0] == '/')
            suffix = uri.substr(1);
        else
            suffix = uri;
    }

    if (suffix.find("..") != std::string::npos)
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

    std::vector<std::string> methodsToCheck;
    if (best && !best->allowed_methods.empty())
        methodsToCheck = best->allowed_methods;
    else if (!config.allowed_methods.empty())
        methodsToCheck = config.allowed_methods;

    if (!methodsToCheck.empty() && !isMethodAllowed(methodsToCheck, "DELETE"))
        return make405(request, methodsToCheck);

    struct stat st;
    if (stat(targetPath.c_str(), &st) != 0)
        return createErrorResponse(request, HTTP_NOT_FOUND);

    if (best && http_response_helpers::isFastCgiRequest(best, targetPath))
    {
        FastCgiClient fcgi(request, config, *best, targetPath);
        return fcgi.execute();
    }

    if (S_ISDIR(st.st_mode))
        return createErrorResponse(request, HTTP_FORBIDDEN);

    if (access(targetPath.c_str(), W_OK) != 0)
        return createErrorResponse(request, HTTP_FORBIDDEN);

    if (remove(targetPath.c_str()))
        return createErrorResponse(request, HTTP_INTERNAL_SERVER_ERROR);

    const std::string okBody = "<html><body><h1>200 OK</h1><p>Deleted</p></body></html>";

    std::ostringstream resp;
    resp << request.getHttpVersion() << " 200 OK\r\n";
    resp << "Content-Type: text/html; charset=UTF-8\r\n";
    resp << "Content-Length: " << okBody.size() << "\r\n";
    resp << "Connection: close\r\n\r\n";
    resp << okBody;

    return resp.str();
}
