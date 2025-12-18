#pragma once

#include "HttpRequest.hpp"
#include "Config.hpp"

#include <string>
#include <map>

class FastCgiClient
{
public:
    FastCgiClient(const HttpRequest &request,
                  const ServerConfig &server_config,
                  const LocationConfig &location_config,
                  const std::string &script_path);

    std::string execute();

private:
    // Parsing and setup
    bool parseEndpoint();
    std::map<std::string, std::string> buildParams() const;

    // Low level protocol helpers
    bool sendBeginRequest(int fd) const;
    bool sendParams(int fd, const std::map<std::string, std::string> &params) const;
    bool sendStdin(int fd, const std::string &body) const;
    std::string readResponse(int fd) const;

    // Utility
    std::string buildError(int code, const std::string &reason, const std::string &body) const;
    bool writeAll(int fd, const void *buf, size_t len) const;

private:
    const HttpRequest &req;
    const ServerConfig &server;
    const LocationConfig &location;
    std::string script;
    std::string host;
    int port;
};
