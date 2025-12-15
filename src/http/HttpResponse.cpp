#include "HttpResponse.hpp"
#include "Config.hpp"
#include "macros.hpp"
#include "FastCgiClient.hpp"
#include "FastCgiClient.hpp"
#include <dirent.h>

namespace {

bool isFastCgiRequest(const LocationConfig *loc, const std::string &path)
{
    if (!loc || loc->fastcgi_pass.empty() || path.empty())
        return false;

    std::string::size_type dot = path.rfind('.');
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

} // namespace

// Helper: map status code to reason phrase
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
int HttpResponse::getStatusCode() const { return statusCode; }
const std::string& HttpResponse::getReasonPhrase() const { return reasonPhrase; }
const std::string& HttpResponse::getHeader(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it = headers.find(key);
    if (it != headers.end()) {
        return it->second;
    }
    static const std::string empty = "";
    return empty;
}
const std::string& HttpResponse::getBody() const { return body; }
const std::map<std::string, std::string>& HttpResponse::getHeaders() const { return headers; }

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
    std::ostringstream body_ss;
    body_ss << "<html><head><title>" << errorCode << " " << reason
            << "</title></head><body><h1>" << errorCode << " " << reason
            << "</h1></body></html>";
    const std::string errBody = body_ss.str();

    std::ostringstream resp;
    resp << request.getHttpVersion() << " " << errorCode << " " << reason << "\r\n";
    resp << "Content-Type: text/html; charset=UTF-8\r\n";
    resp << "Content-Length: " << errBody.size() << "\r\n";
    resp << "Connection: close\r\n\r\n";
    resp << errBody;
    return resp.str();
}

// POST handler: validates, resolves target path from location, writes body, returns 201
const std::string HttpResponse::createPostResponse(
    const HttpRequest &request,
    const ServerConfig & config
) const
{
    // Strip query
    std::string uri = request.getUri();
    std::string::size_type qpos = uri.find('?');
    if (qpos != std::string::npos) uri = uri.substr(0, qpos);

    // Match location by longest prefix
    const LocationConfig* best = NULL;
    size_t bestLen = 0;
    for (size_t i = 0; i < config.locations.size(); ++i) {
        const std::string &p = config.locations[i].location;
        if (!p.empty()
            && uri.compare(0, p.size(), p) == 0
            && p.size() > bestLen) {
            best = &config.locations[i];
            bestLen = p.size();
        }
    }
    if (!best) {
        std::ostringstream resp; const std::string msg = "<html><body><h1>405 Method Not Allowed</h1><p>No matching location</p></body></html>";
        resp << request.getHttpVersion() << " 405 Method Not Allowed\r\n";
        resp << "Allow: GET\r\n";
        resp << "Content-Type: text/html; charset=UTF-8\r\n";
        resp << "Content-Length: " << msg.size() << "\r\n";
        resp << "Connection: close\r\n\r\n" << msg; return resp.str();
    }
    bool postAllowed = false;
    for (size_t i = 0; i < best->allowed_methods.size(); ++i) {
        if (best->allowed_methods[i] == "POST") { postAllowed = true; break; }
    }
    if (!postAllowed) {
        std::ostringstream resp;
        const std::string msg = "<html><body><h1>405 Method Not Allowed</h1></body></html>";

        resp << request.getHttpVersion() << " 405 Method Not Allowed\r\n";

        if (!best->allowed_methods.empty()) {
            std::ostringstream allow;
            allow << "Allow: ";
            for (size_t i = 0; i < best->allowed_methods.size(); ++i) {
                if (i) allow << ", ";
                allow << best->allowed_methods[i];
            }
            allow << "\r\n";
            resp << allow.str();
        }

        resp << "Content-Type: text/html; charset=UTF-8\r\n";
        resp << "Content-Length: " << msg.size() << "\r\n";
        resp << "Connection: close\r\n\r\n";
        resp << msg;
        return resp.str();
    }

    // Content-Length required
    const std::map<std::string, std::string> &hdrs = request.getHeaders();
    std::map<std::string, std::string>::const_iterator it = hdrs.find("Content-Length");
    if (it == hdrs.end()) it = hdrs.find("content-length");
    if (it == hdrs.end()) {
        std::ostringstream resp; const std::string msg = "<html><body><h1>411 Length Required</h1></body></html>";
        resp << request.getHttpVersion() << " 411 Length Required\r\n";
        resp << "Content-Type: text/html; charset=UTF-8\r\n";
        resp << "Content-Length: " << msg.size() << "\r\n";
        resp << "Connection: close\r\n\r\n" << msg; return resp.str();
    }

    unsigned long content_length = 0;
    {
        std::string cl = it->second;
        while (!cl.empty() && (cl[0]==' '||cl[0]=='\t')) cl.erase(0,1);
        while (!cl.empty() && (cl[cl.size()-1]==' '||cl[cl.size()-1]=='\t')) cl.erase(cl.size()-1,1);
        for (size_t i=0;i<cl.size();++i) {
            if (cl[i]<'0'||cl[i]>'9') { content_length = 0; break; }
        }
        std::istringstream iss(cl);
        iss >> content_length;
    }

    const std::string &bodyRef = request.getBody();
    if (config.client_max_body_size > 0) {
        if (content_length > config.client_max_body_size || bodyRef.size() > config.client_max_body_size) {
            std::ostringstream resp; const std::string msg = "<html><body><h1>413 Payload Too Large</h1></body></html>";
            resp << request.getHttpVersion() << " 413 Payload Too Large\r\n";
            resp << "Content-Type: text/html; charset=UTF-8\r\n";
            resp << "Content-Length: " << msg.size() << "\r\n";
            resp << "Connection: close\r\n\r\n" << msg; return resp.str();
        }
    }
    if (bodyRef.size() < content_length) {
        std::ostringstream resp; const std::string msg = "<html><body><h1>400 Bad Request</h1><p>Incomplete body</p></body></html>";
        resp << request.getHttpVersion() << " 400 Bad Request\r\n";
        resp << "Content-Type: text/html; charset=UTF-8\r\n";
        resp << "Content-Length: " << msg.size() << "\r\n";
        resp << "Connection: close\r\n\r\n" << msg; return resp.str();
    }

    // Base dir resolution
    std::string baseDir;
    if (!best->upload_dir.empty()) baseDir = best->upload_dir;
    else if (!best->path.empty()) baseDir = best->path;
    else {
        if (best->location == "/") baseDir = request.getRoot();
        else {
            std::string loc = best->location;
            if (!loc.empty() && loc[0]=='/') loc.erase(0,1);
            baseDir = request.getRoot() + "/" + loc;
        }
    }

    std::string suffix = uri.substr(bestLen);
    if (!suffix.empty() && suffix[0]=='/') suffix.erase(0,1);
    if (suffix.empty() || suffix.find("..") != std::string::npos) {
        std::ostringstream resp; const std::string msg = "<html><body><h1>400 Bad Request</h1><p>Invalid target path</p></body></html>";
        resp << request.getHttpVersion() << " 400 Bad Request\r\n";
        resp << "Content-Type: text/html; charset=UTF-8\r\n";
        resp << "Content-Length: " << msg.size() << "\r\n";
        resp << "Connection: close\r\n\r\n" << msg; return resp.str();
    }

    std::string targetPath = baseDir;
    if (!targetPath.empty() && targetPath[targetPath.size() - 1] != '/') {
        targetPath += "/";
    }
    targetPath += suffix;
    if (isFastCgiRequest(best, targetPath))
    {
        struct stat st;
        if (stat(targetPath.c_str(), &st) != 0)
            return createErrorResponse(request, HTTP_NOT_FOUND);
        if (access(targetPath.c_str(), R_OK) != 0)
            return createErrorResponse(request, HTTP_FORBIDDEN);

        FastCgiClient fcgi(request, config, *best, targetPath);
        return fcgi.execute();
    }

    int wfd = open(targetPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (wfd < 0) {
        std::ostringstream resp; const std::string msg = "<html><body><h1>500 Internal Server Error</h1><p>Cannot open target</p></body></html>";
        resp << request.getHttpVersion() << " 500 Internal Server Error\r\n";
        resp << "Content-Type: text/html; charset=UTF-8\r\n";
        resp << "Content-Length: " << msg.size() << "\r\n";
        resp << "Connection: close\r\n\r\n" << msg; return resp.str();
    }
    ssize_t toWrite = static_cast<ssize_t>(content_length);
    ssize_t written = 0;
    const char* data = bodyRef.data();
    while (written < toWrite) {
        ssize_t n = write(wfd, data + written, toWrite - written);
        if (n < 0) {
            close(wfd);
            unlink(targetPath.c_str());
            std::ostringstream resp;
            const std::string msg = "<html><body><h1>500 Internal Server Error</h1><p>Write failed</p></body></html>";
            resp << request.getHttpVersion() << " 500 Internal Server Error\r\n";
            resp << "Content-Type: text/html; charset=UTF-8\r\n";
            resp << "Content-Length: " << msg.size() << "\r\n";
            resp << "Connection: close\r\n\r\n" << msg;
            return resp.str();
        }
        written += n;
    }
    close(wfd);

    std::ostringstream resp;
    const std::string okBody = "<html><body><h1>201 Created</h1></body></html>";
    resp << request.getHttpVersion() << " 201 Created\r\n";
    resp << "Location: " << uri << "\r\n";
    resp << "Content-Type: text/html; charset=UTF-8\r\n";
    resp << "Content-Length: " << okBody.size() << "\r\n";
    resp << "Connection: close\r\n\r\n";
    resp << okBody;
    return resp.str();
}

const std::string HttpResponse::createDeleteResponse(const HttpRequest &request, const ServerConfig &config) const
{
    (void) config;
    const std::string body = "<html><body><h1>501 Not Implemented</h1></body></html>";
    std::ostringstream resp;
    resp << request.getHttpVersion() << " 501 Not Implemented\r\n";
    resp << "Content-Type: text/html; charset=UTF-8\r\n";
    resp << "Content-Length: " << body.size() << "\r\n";
    resp << "Connection: close\r\n\r\n" << body; return resp.str();
}

// GET handler: maps URI to a file using location mapping and serves it
const std::string HttpResponse::createGetResponse(const HttpRequest &request, const ServerConfig &config)
{
    struct stat fileStat;
    int fd;
    std::string path;
    char buffer[BUFF_SIZE];
    ssize_t n;

    std::string uri = request.getUri();
    std::string::size_type qpos = uri.find('?');
    if (qpos != std::string::npos)
        uri = uri.substr(0, qpos);

    const LocationConfig* best = NULL;
    size_t bestLen = 0;
    for (size_t i=0; i<config.locations.size(); ++i) {
        const std::string &p = config.locations[i].location;
        if (!p.empty() && uri.compare(0, p.size(), p) == 0 && p.size() > bestLen) 
        {
            best = &config.locations[i];
            bestLen = p.size();
        }
    }
    if (best) {
        bool getAllowed = false;
        for (size_t i=0; i<best->allowed_methods.size(); ++i) {
            if (best->allowed_methods[i] == "GET") 
            {
                getAllowed = true;
                break; 
            }
        }
        if (!getAllowed) {
            std::ostringstream resp;
            const std::string msg = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
            resp << request.getHttpVersion() << " 405 Method Not Allowed\r\n";
            if (!best->allowed_methods.empty()) {
                std::ostringstream allow;
                allow << "Allow: ";
                for (size_t i=0; i<best->allowed_methods.size(); ++i) {
                    if (i) allow << ", ";
                    allow << best->allowed_methods[i];
                }
                allow << "\r\n";
                resp << allow.str();
            }
            resp << "Content-Type: text/html; charset=UTF-8\r\n";
            resp << "Content-Length: " << msg.size() << "\r\n";
            resp << "Connection: close\r\n\r\n" << msg;
            return resp.str();
        }
    }

    if (uri.find("..") != std::string::npos) {
        return createErrorResponse(request, HTTP_FORBIDDEN); 
    }

    std::string baseDir;
    std::string suffix;
    if (best) {
        if (!best->path.empty())
            baseDir = best->path;
        else if (!best->upload_dir.empty()) 
            baseDir = best->upload_dir;
        else if (best->location == "/")
            baseDir = request.getRoot();
        else {
            std::string loc = best->location;
            if(!loc.empty() && loc[0]=='/') 
                loc.erase(0,1);
            baseDir = request.getRoot()+"/"+loc;
        }
        suffix = uri.substr(bestLen);
        if(!suffix.empty() && suffix[0]=='/')
            suffix.erase(0,1);
    } else {
        baseDir = request.getRoot();
        if(!uri.empty() && uri[0]=='/')
            suffix = uri.substr(1); 
        else
            suffix = uri;
    }

    bool isDirectoryRequest = (suffix.empty() || suffix[suffix.size()-1] == '/');
    std::string dirPath = baseDir;
    if (!dirPath.empty() && dirPath[dirPath.size() - 1] != '/')
        dirPath += "/";
    dirPath += suffix;
    if (!dirPath.empty() && dirPath[dirPath.size() - 1] != '/')
        dirPath += "/";

    std::string tryPath;
    if (isDirectoryRequest) {
        for (size_t i = 0; i < config.index_files.size(); ++i) {
            tryPath = dirPath + config.index_files[i];
            if (stat(tryPath.c_str(), &fileStat) == 0 && !S_ISDIR(fileStat.st_mode)) {
                path = tryPath;
                break;
            }
        }
        if (path.empty()) {
            struct stat dirStat;
            if (stat(dirPath.c_str(), &dirStat) != 0 || !S_ISDIR(dirStat.st_mode)) {
                return createErrorResponse(request, HTTP_NOT_FOUND);
            }
            if (best && best->autoindex) {
                DIR *d = opendir(dirPath.c_str());
                if (!d) return createErrorResponse(request, HTTP_FORBIDDEN);

                std::ostringstream listing;
                listing << "<html><body><h1>Index of " << uri << "</h1><ul>";
                struct dirent *ent;
                while ((ent = readdir(d)) != NULL) {
                    const char *name = ent->d_name;
                    if (!name) continue;
                    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;
                    listing << "<li><a href=\"" << uri;
                    if (!uri.empty() && uri[uri.size() - 1] != '/') listing << "/";
                    listing << name << "\">" << name << "</a></li>";
                }
                closedir(d);
                listing << "</ul></body></html>";

                std::ostringstream resp;
                std::string bodyContent = listing.str();
                resp << request.getHttpVersion() << " 200 OK\r\n";
                resp << "Content-Type: text/html; charset=UTF-8\r\n";
                resp << "Content-Length: " << bodyContent.size() << "\r\n";
                resp << "Connection: close\r\n\r\n";
                resp << bodyContent;
                return resp.str();
            }
            return createErrorResponse(request, HTTP_FORBIDDEN);
        }
    } else {
        path = dirPath;
        if (!path.empty() && path[path.size()-1] == '/')
            path.erase(path.size()-1);
    }

    if (stat(path.c_str(), &fileStat) != 0) { return createErrorResponse(request, HTTP_NOT_FOUND); }
    if (S_ISDIR(fileStat.st_mode)) {
        if (uri.empty() || uri[uri.size() - 1] != '/') {
            std::ostringstream resp;
            resp << request.getHttpVersion() << " 301 Moved Permanently\r\n";
            resp << "Location: " << uri << "/\r\n";
            resp << "Content-Length: 0\r\n";
            resp << "Connection: close\r\n\r\n";
            return resp.str();
        }
        return createErrorResponse(request, HTTP_FORBIDDEN);
    }
    if (access(path.c_str(), R_OK) != 0) { return createErrorResponse(request, HTTP_FORBIDDEN); }

    if (isFastCgiRequest(best, path))
    {
        FastCgiClient fcgi(request, config, *best, path);
        return fcgi.execute();
    }

    fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) { return createErrorResponse(request, HTTP_INTERNAL_SERVER_ERROR); }
    body.clear();
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        body.append(buffer, n);
    }
    if (n < 0) { close(fd); return createErrorResponse(request, HTTP_INTERNAL_SERVER_ERROR); }
    close(fd);

    std::string ctype = "application/octet-stream";
    std::string::size_type dot = path.rfind('.');
    if (dot != std::string::npos) {
        std::string ext = path.substr(dot);
        if (ext == ".html" || ext == ".htm") ctype = "text/html; charset=UTF-8";
        else if (ext == ".css") ctype = "text/css";
        else if (ext == ".js") ctype = "application/javascript";
        else if (ext == ".json") ctype = "application/json";
        else if (ext == ".png") ctype = "image/png";
        else if (ext == ".svg") ctype = "image/svg+xml";
        else if (ext == ".jpg" || ext == ".jpeg") ctype = "image/jpeg";
        else if (ext == ".gif") ctype = "image/gif";
    }
    setContentType(ctype);

    createOkResponse(request); return fullResponse;
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
    resp << "Connection: close\r\n\r\n" << body; return resp.str();
}

// Entry point: routes to method-specific handler
std::string HttpResponse::createResponse(
    const HttpRequest &request,
    const ServerConfig& config)
{
    HttpResponse response; const std::string& method = request.getMethod();
    if (method == "GET") return response.createGetResponse(request, config);
    else if (method == "POST") return response.createPostResponse(request, config);
    else if (method == "DELETE") return response.createDeleteResponse(request, config);
    else return response.createUnknowResponse(request, config);
}
