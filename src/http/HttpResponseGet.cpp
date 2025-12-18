#include "HttpResponse.hpp"

#include "FastCgiClient.hpp"
#include "HttpResponseHelpers.hpp"
#include "macros.hpp"

#include <dirent.h>
#include <iomanip>
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

std::string humanReadableSize(off_t bytes)
{
    if (bytes < 0)
        return "-";

    const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    double size = static_cast<double>(bytes);
    size_t unit = 0;

    while (size >= 1024.0 && unit < (sizeof(units) / sizeof(units[0])) - 1)
    {
        size /= 1024.0;
        ++unit;
    }

    std::ostringstream oss;
    if (unit == 0)
        oss << static_cast<long long>(bytes) << ' ' << units[unit];
    else
    {
        oss << std::fixed << std::setprecision(size >= 100 ? 0 : (size >= 10 ? 1 : 2));
        oss << size << ' ' << units[unit];
    }
    return oss.str();
}

std::string formatTimestamp(time_t ts)
{
    struct tm *tm = localtime(&ts);
    if (!tm)
        return "-";

    char buffer[64];
    if (!strftime(buffer, sizeof(buffer), "%d %b %Y • %H:%M", tm))
        return "-";
    return buffer;
}

std::string buildBreadcrumb(const std::string &uri)
{
    std::ostringstream nav;
    nav << "<nav class=\"crumbs\"><a href=\"/\">root</a>";

    std::string trimmed = uri;
    if (!trimmed.empty() && trimmed[0] == '/')
        trimmed.erase(0, 1);
    if (!trimmed.empty() && trimmed[trimmed.size() - 1] == '/')
        trimmed.erase(trimmed.size() - 1);

    if (trimmed.empty())
    {
        nav << "</nav>";
        return nav.str();
    }

    std::istringstream iss(trimmed);
    std::string segment;
    std::string href;
    while (std::getline(iss, segment, '/'))
    {
        if (segment.empty())
            continue;
        href += "/";
        href += segment;
        nav << "<span>/</span><a href=\"" << href << "\">" << segment << "</a>";
    }
    nav << "</nav>";
    return nav.str();
}

std::string buildAutoIndex(const std::string &uri, const std::string &dirPath)
{
    DIR *d = opendir(dirPath.c_str());
    if (!d)
        return "";

    std::ostringstream listing;
    listing << "<!doctype html><html><head><meta charset=\"utf-8\">"
        << "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        << "<title>Index of " << uri << "</title>"
        << "<style>:root{--bg:#04060f;--accent:#8c6ff7;--accent2:#5de0e6;--panel:rgba(13,17,38,.85);--text:#f4f6ff;--muted:#b8bfda;}"
        << "body{font-family:'Space Grotesk',system-ui,sans-serif;background:radial-gradient(circle at 20% 20%,rgba(92,69,255,.4),transparent 50%),radial-gradient(circle at 80% 0,rgba(93,224,230,.3),transparent 45%),var(--bg);color:var(--text);min-height:100vh;margin:0;padding:32px;display:flex;justify-content:center;}"
        << ".backdrop{width:100%;max-width:960px;background:linear-gradient(135deg,rgba(23,27,62,.95),rgba(10,12,24,.9));border:1px solid rgba(255,255,255,.08);border-radius:24px;box-shadow:0 25px 80px rgba(0,0,0,.45);backdrop-filter:blur(14px);overflow:hidden;}"
        << "header{padding:32px;border-bottom:1px solid rgba(255,255,255,.05);}"
        << "h1{margin:0;font-size:28px;font-weight:600;letter-spacing:.02em;}"
        << ".subtitle{color:var(--muted);margin-top:8px;font-size:15px;}"
        << ".crumbs{margin-top:18px;font-size:13px;text-transform:uppercase;letter-spacing:.2em;color:var(--muted);}"
        << ".crumbs a{color:var(--text);text-decoration:none;font-weight:500;}"
        << ".crumbs span{margin:0 8px;color:rgba(255,255,255,.4);}"
        << "main{padding:32px;}"
        << ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(240px,1fr));gap:18px;}"
        << ".item{display:block;padding:20px;border-radius:18px;background:var(--panel);border:1px solid rgba(255,255,255,.06);text-decoration:none;color:var(--text);transition:transform .2s ease, border-color .2s ease, background .2s ease;}"
        << ".item:hover{transform:translateY(-4px);border-color:rgba(255,255,255,.25);background:rgba(25,31,64,.9);}"
        << ".item.dir{border-color:rgba(92,69,255,.3);}"
        << ".name{font-size:17px;font-weight:600;margin-bottom:10px;display:flex;gap:8px;align-items:center;}"
        << ".badge{font-size:11px;padding:2px 8px;border-radius:999px;background:rgba(255,255,255,.1);text-transform:uppercase;letter-spacing:.08em;}"
        << ".meta{display:flex;justify-content:space-between;color:var(--muted);font-size:13px;}"
        << ".empty{margin:40px auto;text-align:center;color:var(--muted);font-size:15px;}"
        << "footer{padding:18px 32px;border-top:1px solid rgba(255,255,255,.05);color:var(--muted);font-size:13px;text-align:right;}"
        << "@media(max-width:600px){body{padding:16px;}header,main{padding:24px;} .grid{grid-template-columns:repeat(auto-fit,minmax(180px,1fr));}}"
        << "</style>"
        << "</head><body><div class=\"backdrop\"><header><h1>Index of " << uri << "</h1>"
        << "<p class=\"subtitle\">Browse files served by webserv</p>"
        << buildBreadcrumb(uri)
        << "</header><main><div class=\"grid\">";

    struct dirent *ent;
    bool hasEntries = false;
    while ((ent = readdir(d)) != NULL)
    {
        const char *name = ent->d_name;
        if (!name)
            continue;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
            continue;

        std::string href = uri;
        if (!href.empty() && href[href.size() - 1] != '/')
            href += '/';
        href += name;

        std::string fullPath = dirPath + name;
        struct stat entStat;
        bool statOk = false;
        std::string mtimeStr = "-";
        bool isDir = false;

        if (stat(fullPath.c_str(), &entStat) == 0)
        {
            statOk = true;
            if (S_ISDIR(entStat.st_mode))
                isDir = true;

            mtimeStr = formatTimestamp(entStat.st_mtime);
        }

        const std::string displayName = isDir ? std::string(name) + "/" : std::string(name);
        const std::string badge = isDir ? "folder" : "file";
        const std::string sizeLabel = isDir ? "Directory" : (statOk ? humanReadableSize(entStat.st_size) : "-");

        listing << "<a class=\"item " << (isDir ? "dir" : "file") << "\" href=\"" << href << "\">"
            << "<div class=\"name\">" << displayName
            << "<span class=\"badge\">" << badge << "</span></div>"
            << "<div class=\"meta\"><span>" << sizeLabel << "</span><span>" << mtimeStr << "</span></div>"
            << "</a>";
        hasEntries = true;
    }

    closedir(d);

    if (!hasEntries)
        listing << "<p class=\"empty\">This folder is feeling lonely.</p>";

    listing << "</div></main><footer>autoindex • webserv</footer></div></body></html>";

    return listing.str();
}

std::string contentTypeFromPath(const std::string &path)
{
    std::string ctype = "application/octet-stream";

    const std::string::size_type dot = path.rfind('.');
    if (dot == std::string::npos)
        return ctype;

    const std::string ext = path.substr(dot);
    if (ext == ".html" || ext == ".htm") ctype = "text/html; charset=UTF-8";
    else if (ext == ".css") ctype = "text/css";
    else if (ext == ".js") ctype = "application/javascript";
    else if (ext == ".json") ctype = "application/json";
    else if (ext == ".png") ctype = "image/png";
    else if (ext == ".svg") ctype = "image/svg+xml";
    else if (ext == ".jpg" || ext == ".jpeg") ctype = "image/jpeg";
    else if (ext == ".gif") ctype = "image/gif";

    return ctype;
}

} // namespace

const std::string HttpResponse::createGetResponse(const HttpRequest &request, const ServerConfig &config)
{
    struct stat fileStat;

    std::string uri = http_response_helpers::stripQuery(request.getUri());

    size_t bestLen = 0;
    const LocationConfig *best = matchBestLocation(config, uri, bestLen);

    if (best && !best->allowed_methods.empty() && !isMethodAllowed(best->allowed_methods, "GET"))
    {
        const std::string msg = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        std::ostringstream resp;
        resp << request.getHttpVersion() << " 405 Method Not Allowed\r\n";
        resp << http_response_helpers::buildAllowHeader(best->allowed_methods);
        resp << "Content-Type: text/html; charset=UTF-8\r\n";
        resp << "Content-Length: " << msg.size() << "\r\n";
        resp << "Connection: close\r\n\r\n";
        resp << msg;
        return resp.str();
    }

    if (uri.find("..") != std::string::npos)
        return createErrorResponse(request, HTTP_FORBIDDEN);

    std::string baseDir;
    std::string suffix;

    if (best)
    {
        if (!best->path.empty())
            baseDir = best->path;
        else if (!best->upload_dir.empty())
            baseDir = best->upload_dir;
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

    const bool isDirReq = (suffix.empty() || suffix[suffix.size() - 1] == '/');

    std::string dirPath = baseDir;
    if (!dirPath.empty() && dirPath[dirPath.size() - 1] != '/')
        dirPath += "/";
    dirPath += suffix;
    if (!dirPath.empty() && dirPath[dirPath.size() - 1] != '/')
        dirPath += "/";

    std::string path;

    if (isDirReq)
    {
        for (size_t i = 0; i < config.index_files.size(); ++i)
        {
            const std::string tryPath = dirPath + config.index_files[i];
            if (stat(tryPath.c_str(), &fileStat) == 0 && !S_ISDIR(fileStat.st_mode))
            {
                path = tryPath;
                break;
            }
        }

        if (path.empty())
        {
            struct stat dirStat;
            if (stat(dirPath.c_str(), &dirStat) != 0 || !S_ISDIR(dirStat.st_mode))
                return createErrorResponse(request, HTTP_NOT_FOUND);

            if (best && best->autoindex)
            {
                const std::string html = buildAutoIndex(uri, dirPath);
                if (html.empty())
                    return createErrorResponse(request, HTTP_FORBIDDEN);

                std::ostringstream resp;
                resp << request.getHttpVersion() << " 200 OK\r\n";
                resp << "Content-Type: text/html; charset=UTF-8\r\n";
                resp << "Content-Length: " << html.size() << "\r\n";
                resp << "Connection: close\r\n\r\n";
                resp << html;
                return resp.str();
            }

            return createErrorResponse(request, HTTP_FORBIDDEN);
        }
    }
    else
    {
        path = dirPath;
        if (!path.empty() && path[path.size() - 1] == '/')
            path.erase(path.size() - 1);
    }

    if (stat(path.c_str(), &fileStat) != 0)
        return createErrorResponse(request, HTTP_NOT_FOUND);

    if (S_ISDIR(fileStat.st_mode))
    {
        if (uri.empty() || uri[uri.size() - 1] != '/')
        {
            std::ostringstream resp;
            resp << request.getHttpVersion() << " 301 Moved Permanently\r\n";
            resp << "Location: " << uri << "/\r\n";
            resp << "Content-Length: 0\r\n";
            resp << "Connection: close\r\n\r\n";
            return resp.str();
        }
        return createErrorResponse(request, HTTP_FORBIDDEN);
    }

    if (access(path.c_str(), R_OK) != 0)
        return createErrorResponse(request, HTTP_FORBIDDEN);

    if (best && http_response_helpers::isFastCgiRequest(best, path))
    {
        FastCgiClient fcgi(request, config, *best, path);
        return fcgi.execute();
    }

    const int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return createErrorResponse(request, HTTP_INTERNAL_SERVER_ERROR);

    char buffer[BUFF_SIZE];
    ssize_t n;

    body.clear();
    while ((n = read(fd, buffer, sizeof(buffer))) > 0)
        body.append(buffer, n);

    if (n < 0)
    {
        close(fd);
        return createErrorResponse(request, HTTP_INTERNAL_SERVER_ERROR);
    }

    close(fd);

    setContentType(contentTypeFromPath(path));

    createOkResponse(request);
    return fullResponse;
}
