#include "FastCgiClient.hpp"
#include "macros.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <sstream>
#include <vector>

namespace {

const unsigned char FCGI_VERSION_1 = 1;
const unsigned char FCGI_BEGIN_REQUEST = 1;
const unsigned char FCGI_END_REQUEST = 3;
const unsigned char FCGI_PARAMS = 4;
const unsigned char FCGI_STDIN = 5;
const unsigned char FCGI_STDOUT = 6;
const unsigned char FCGI_STDERR = 7;

const unsigned short FCGI_RESPONDER = 1;

struct FcgiHeader
{
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
};

struct FcgiBeginRequestBody
{
    unsigned char roleB1;
    unsigned char roleB0;
    unsigned char flags;
    unsigned char reserved[5];
};

std::string toString(size_t value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

bool encodeNameValue(const std::string &name,
                     const std::string &value,
                     std::vector<unsigned char> &out)
{
    size_t nlen = name.size();
    size_t vlen = value.size();

    if (nlen < 128)
        out.push_back(static_cast<unsigned char>(nlen));
    else
    {
        out.push_back(static_cast<unsigned char>((nlen >> 24) | 0x80));
        out.push_back(static_cast<unsigned char>((nlen >> 16) & 0xFF));
        out.push_back(static_cast<unsigned char>((nlen >> 8) & 0xFF));
        out.push_back(static_cast<unsigned char>(nlen & 0xFF));
    }

    if (vlen < 128)
        out.push_back(static_cast<unsigned char>(vlen));
    else
    {
        out.push_back(static_cast<unsigned char>((vlen >> 24) | 0x80));
        out.push_back(static_cast<unsigned char>((vlen >> 16) & 0xFF));
        out.push_back(static_cast<unsigned char>((vlen >> 8) & 0xFF));
        out.push_back(static_cast<unsigned char>(vlen & 0xFF));
    }

    out.insert(out.end(), name.begin(), name.end());
    out.insert(out.end(), value.begin(), value.end());
    return true;
}

} // namespace

FastCgiClient::FastCgiClient(const HttpRequest &request,
                             const ServerConfig &server_config,
                             const LocationConfig &location_config,
                             const std::string &script_path)
    : req(request)
    , server(server_config)
    , location(location_config)
    , script(script_path)
    , host()
    , port(0)
{
}

bool FastCgiClient::parseEndpoint()
{
    if (location.fastcgi_pass.empty())
        return false;

    std::string::size_type colon = location.fastcgi_pass.find(':');
    if (colon == std::string::npos)
        return false;

    host = location.fastcgi_pass.substr(0, colon);
    std::string port_str = location.fastcgi_pass.substr(colon + 1);
    std::istringstream iss(port_str);
    iss >> port;
    if (!iss || port <= 0 || port > 65535)
        return false;
    return true;
}

bool FastCgiClient::writeAll(int fd, const void *buf, size_t len) const
{
    const char *p = static_cast<const char *>(buf);
    size_t left = len;
    while (left > 0)
    {
        ssize_t n = send(fd, p, left, 0);
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            return false;
        }
        p += n;
        left -= static_cast<size_t>(n);
    }
    return true;
}

bool FastCgiClient::sendBeginRequest(int fd) const
{
    FcgiHeader header;
    std::memset(&header, 0, sizeof(header));
    header.version = FCGI_VERSION_1;
    header.type = FCGI_BEGIN_REQUEST;
    header.requestIdB1 = 0;
    header.requestIdB0 = 1;
    header.contentLengthB1 = 0;
    header.contentLengthB0 = sizeof(FcgiBeginRequestBody);

    FcgiBeginRequestBody body;
    std::memset(&body, 0, sizeof(body));
    body.roleB1 = 0;
    body.roleB0 = static_cast<unsigned char>(FCGI_RESPONDER);
    body.flags = 0;

    return writeAll(fd, &header, sizeof(header))
        && writeAll(fd, &body, sizeof(body));
}

bool FastCgiClient::sendParams(int fd, const std::map<std::string, std::string> &params) const
{
    std::vector<unsigned char> buffer;
    buffer.reserve(1024);

    std::map<std::string, std::string>::const_iterator it = params.begin();
    while (it != params.end())
    {
        encodeNameValue(it->first, it->second, buffer);
        ++it;
        if (buffer.size() > 60000 || it == params.end())
        {
            FcgiHeader header;
            std::memset(&header, 0, sizeof(header));
            header.version = FCGI_VERSION_1;
            header.type = FCGI_PARAMS;
            header.requestIdB1 = 0;
            header.requestIdB0 = 1;
            header.contentLengthB1 = static_cast<unsigned char>((buffer.size() >> 8) & 0xFF);
            header.contentLengthB0 = static_cast<unsigned char>(buffer.size() & 0xFF);
            header.paddingLength = 0;

            if (!writeAll(fd, &header, sizeof(header)))
                return false;
            if (!buffer.empty() && !writeAll(fd, &buffer[0], buffer.size()))
                return false;
            buffer.clear();
        }
    }

    // Terminator params record
    FcgiHeader header;
    std::memset(&header, 0, sizeof(header));
    header.version = FCGI_VERSION_1;
    header.type = FCGI_PARAMS;
    header.requestIdB1 = 0;
    header.requestIdB0 = 1;
    header.contentLengthB1 = 0;
    header.contentLengthB0 = 0;
    header.paddingLength = 0;
    return writeAll(fd, &header, sizeof(header));
}

bool FastCgiClient::sendStdin(int fd, const std::string &body) const
{
    size_t offset = 0;
    const size_t total = body.size();
    while (offset < total)
    {
        size_t chunk = total - offset;
        if (chunk > 65535)
            chunk = 65535;

        FcgiHeader header;
        std::memset(&header, 0, sizeof(header));
        header.version = FCGI_VERSION_1;
        header.type = FCGI_STDIN;
        header.requestIdB1 = 0;
        header.requestIdB0 = 1;
        header.contentLengthB1 = static_cast<unsigned char>((chunk >> 8) & 0xFF);
        header.contentLengthB0 = static_cast<unsigned char>(chunk & 0xFF);
        header.paddingLength = 0;

        if (!writeAll(fd, &header, sizeof(header)))
            return false;
        if (!writeAll(fd, body.data() + offset, chunk))
            return false;
        offset += chunk;
    }

    // Terminator STDIN record
    FcgiHeader header;
    std::memset(&header, 0, sizeof(header));
    header.version = FCGI_VERSION_1;
    header.type = FCGI_STDIN;
    header.requestIdB1 = 0;
    header.requestIdB0 = 1;
    header.contentLengthB1 = 0;
    header.contentLengthB0 = 0;
    header.paddingLength = 0;
    return writeAll(fd, &header, sizeof(header));
}

std::map<std::string, std::string> FastCgiClient::buildParams() const
{
    std::map<std::string, std::string> env;

    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["REQUEST_METHOD"] = req.getMethod();
    env["SERVER_PROTOCOL"] = req.getHttpVersion();
    env["SERVER_NAME"] = server.server_name;
    env["SERVER_PORT"] = toString(server.port);

    const std::string uri = req.getUri();
    std::string query;
    std::string path_info = uri;
    std::string::size_type qpos = uri.find('?');
    if (qpos != std::string::npos)
    {
        query = uri.substr(qpos + 1);
        path_info = uri.substr(0, qpos);
    }
    env["QUERY_STRING"] = query;
    env["SCRIPT_NAME"] = path_info;
    env["PATH_INFO"] = path_info;
    env["SCRIPT_FILENAME"] = script;
    env["DOCUMENT_ROOT"] = server.root;

    const std::map<std::string, std::string> &hdrs = req.getHeaders();
    std::map<std::string, std::string>::const_iterator it = hdrs.find("Content-Type");
    if (it == hdrs.end())
        it = hdrs.find("content-type");
    if (it != hdrs.end())
        env["CONTENT_TYPE"] = it->second;

    it = hdrs.find("Content-Length");
    if (it == hdrs.end())
        it = hdrs.find("content-length");
    if (it != hdrs.end())
        env["CONTENT_LENGTH"] = it->second;
    else
        env["CONTENT_LENGTH"] = toString(req.getBody().size());

    env["REDIRECT_STATUS"] = "200";

    return env;
}

std::string FastCgiClient::readResponse(int fd) const
{
    std::string stdout_data;

    while (true)
    {
        FcgiHeader header;
        ssize_t n = recv(fd, &header, sizeof(header), MSG_WAITALL);
        if (n == 0)
            break;
        if (n < 0)
        {
            if (errno == EINTR)
                continue;
            return "";
        }
        if (n != static_cast<ssize_t>(sizeof(header)))
            return "";

        unsigned short contentLength = static_cast<unsigned short>((header.contentLengthB1 << 8) | header.contentLengthB0);
        unsigned char paddingLength = header.paddingLength;

        if (contentLength > 0)
        {
            std::string payload;
            payload.resize(contentLength);
            ssize_t r = recv(fd, &payload[0], contentLength, MSG_WAITALL);
            if (r != static_cast<ssize_t>(contentLength))
                return "";

            if (header.type == FCGI_STDOUT)
                stdout_data.append(payload);
            // STDERR is ignored for now
        }

        if (paddingLength > 0)
        {
            char pad[256];
            ssize_t r = recv(fd, pad, paddingLength, MSG_WAITALL);
            if (r != static_cast<ssize_t>(paddingLength))
                return "";
        }

        if (header.type == FCGI_END_REQUEST)
            break;
    }

    return stdout_data;
}

std::string FastCgiClient::buildError(int code, const std::string &reason, const std::string &body) const
{
    std::ostringstream resp;
    resp << req.getHttpVersion() << " " << code << " " << reason << "\r\n";
    resp << "Content-Type: text/html; charset=UTF-8\r\n";
    resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Connection: close\r\n\r\n";
    resp << body;
    return resp.str();
}

std::string FastCgiClient::execute()
{
    if (!parseEndpoint())
        return buildError(502, "Bad Gateway", "<html><body><h1>502 Bad Gateway</h1><p>Invalid fastcgi_pass</p></body></html>");

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return buildError(502, "Bad Gateway", "<html><body><h1>502 Bad Gateway</h1><p>socket failed</p></body></html>");

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(port));
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
    {
        close(fd);
        return buildError(502, "Bad Gateway", "<html><body><h1>502 Bad Gateway</h1><p>Invalid FastCGI host</p></body></html>");
    }

    if (connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        close(fd);
        return buildError(504, "Gateway Timeout", "<html><body><h1>504 Gateway Timeout</h1><p>FastCGI connect failed</p></body></html>");
    }

    std::map<std::string, std::string> params = buildParams();
    const std::string body = req.getBody();

    bool ok = sendBeginRequest(fd)
        && sendParams(fd, params)
        && sendStdin(fd, body);

    if (!ok)
    {
        close(fd);
        return buildError(502, "Bad Gateway", "<html><body><h1>502 Bad Gateway</h1><p>FastCGI send failed</p></body></html>");
    }

    std::string responseData = readResponse(fd);
    close(fd);
    if (responseData.empty())
        return buildError(502, "Bad Gateway", "<html><body><h1>502 Bad Gateway</h1><p>Empty FastCGI response</p></body></html>");

    // Split headers/body at first blank line
    std::string::size_type pos = responseData.find("\r\n\r\n");
    std::string::size_type alt = responseData.find("\n\n");
    size_t delimLen = 4;
    if (pos == std::string::npos || (alt != std::string::npos && alt < pos))
    {
        pos = alt;
        delimLen = 2;
    }

    std::string headerBlock;
    std::string bodyBlock;
    if (pos != std::string::npos)
    {
        headerBlock = responseData.substr(0, pos);
        bodyBlock = responseData.substr(pos + delimLen);
    }
    else
    {
        bodyBlock = responseData;
    }

    int statusCode = 200;
    std::string reason = "OK";
    std::map<std::string, std::string> outHeaders;

    std::istringstream hss(headerBlock);
    std::string line;
    while (std::getline(hss, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        std::string::size_type colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        while (!value.empty() && (value[0] == ' ' || value[0] == '\t'))
            value.erase(0, 1);
        if (key == "Status")
        {
            std::istringstream iss(value);
            iss >> statusCode;
            if (iss)
            {
                std::string rest;
                std::getline(iss, rest);
                if (!rest.empty() && rest[0] == ' ')
                    rest.erase(0, 1);
                if (!rest.empty())
                    reason = rest;
            }
        }
        else
        {
            outHeaders[key] = value;
        }
    }

    if (outHeaders.find("Content-Length") == outHeaders.end()
        && outHeaders.find("content-length") == outHeaders.end())
    {
        outHeaders["Content-Length"] = toString(bodyBlock.size());
    }
    if (outHeaders.find("Content-Type") == outHeaders.end()
        && outHeaders.find("content-type") == outHeaders.end())
    {
        outHeaders["Content-Type"] = "text/html; charset=UTF-8";
    }

    std::ostringstream resp;
    resp << req.getHttpVersion() << " " << statusCode << " " << reason << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = outHeaders.begin(); it != outHeaders.end(); ++it)
    {
        resp << it->first << ": " << it->second << "\r\n";
    }
    resp << "Connection: close\r\n\r\n";
    resp << bodyBlock;
    return resp.str();
}
