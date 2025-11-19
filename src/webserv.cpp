// Minimal HTTP server in C++ using BSD sockets

#include <iostream>
#include <vector>

#include "macros.hpp"
#include "Config.hpp"
#include "Server.hpp"

namespace {

LocationConfig buildLocation(const std::string &route,
                             const std::string &path,
                             const std::vector<std::string> &methods)
{
    LocationConfig location;
    location.location = route;
    location.path = path;
    location.allowed_methods = methods;
    location.autoindex = false;
    return location;
}

std::vector<std::string> allow(const std::string &method)
{
    std::vector<std::string> methods;
    methods.push_back(method);
    return methods;
}

ServerConfig buildDefaultConfig()
{
    ServerConfig config;
    config.port = 8080;
    config.server_name = "localhost";
    config.root = "./site1/www";
    config.index_files.push_back("index.html");
    config.client_max_body_size = 1024 * 1024; // 1MB
    config.client_timeout = CLIENT_TIMEOUT;
    config.error_pages[404] = "./site1/www/errors/404.html";
    config.error_pages[500] = "./site1/www/errors/500.html";

    config.locations.push_back(buildLocation("/", config.root, allow("GET")));
    config.locations.push_back(buildLocation("/images", "./site1/www/images", allow("GET")));
    config.locations.push_back(buildLocation("/upload", "./site1/www/uploads", allow("POST")));

    return config;
}

void printStartupBanner(const ServerConfig &config)
{
    std::cout << "Server initialized successfully" << std::endl;
    std::cout << "Open in your browser:" << std::endl;
    std::cout << "  -> http://127.0.0.1:" << config.port << "/" << std::endl;
    std::cout << "  -> http://localhost:" << config.port << "/" << std::endl;
    std::cout << "Serving root: " << config.root << std::endl;
}

} // namespace

Server * g_server = NULL;

int main(int ac, char *av[]) {
    if (ac > 2)
    {
        std::cerr << "Usage: webserv [<config_file_path>]" << std::endl;
        return 1;
    }

    const std::string config_path = (ac == 2) ? av[1] : "";
    if (!config_path.empty())
    {
        std::cout << "Config parsing from file is not implemented yet."
                  << " Falling back to the embedded sample." << std::endl;
    }

    try
    {
        const ServerConfig server_config = buildDefaultConfig();
        Server server(server_config);
        g_server = &server;

        if (!server.init())
        {
            std::cerr << "Failed to init server on port " << server_config.port << std::endl;
            return 1;
        }

        printStartupBanner(server_config);
        server.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Server shutdown finished successfully" << std::endl;
    return 0;
}
