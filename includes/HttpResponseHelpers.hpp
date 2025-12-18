#pragma once

#include "Config.hpp"

#include <sstream>
#include <string>
#include <vector>

namespace http_response_helpers {

inline std::string stripQuery(const std::string &uri)
{
    std::string out = uri;
    const std::string::size_type qpos = out.find('?');

    if (qpos != std::string::npos)
        out = out.substr(0, qpos);
    return out;
}

inline bool isFastCgiRequest(const LocationConfig *loc, const std::string &path)
{
    if (!loc || loc->fastcgi_pass.empty() || path.empty())
        return false;

    const std::string::size_type dot = path.rfind('.');
    if (dot == std::string::npos)
        return false;

    const std::string ext = path.substr(dot);
    for (size_t i = 0; i < loc->cgi_extensions.size(); ++i)
    {
        if (loc->cgi_extensions[i] == ext)
            return true;
    }
    return false;
}

inline std::string buildAllowHeader(const std::vector<std::string> &methods)
{
    std::ostringstream allow;

    allow << "Allow: ";
    for (size_t i = 0; i < methods.size(); ++i)
    {
        if (i)
            allow << ", ";
        allow << methods[i];
    }
    allow << "\r\n";

    return allow.str();
}

} // namespace http_response_helpers
