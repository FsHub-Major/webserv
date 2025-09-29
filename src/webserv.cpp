// Minimal HTTP server in C++ using BSD sockets

#include "webserv.hpp"
#include "macros.hpp"
#include "Config.hpp"
#include "Server.hpp"




int load_port(const std::string& path) {
    std::ifstream cfg(path.c_str());
    std::vector<std::string> tokens;

    if (!cfg.is_open())
    {
        std::cerr << "Failed to open config file: " << path << "\n";
        return -1;
    }
    std::string line;
    while (std::getline(cfg, line))
        tokens = split(line, " =");
    for (size_t i = 0; i < tokens.size(); i++)
    {
        if (tokens[i] == "port")
            return (stringtoi(tokens[i + 1]));
    }
    std::cerr << "No port setting found in config file\n";
    return -1;
}

Server * g_server = NULL;


int main(int ac, char *av[]) {

    (void) av;
    (void)ac;
    try{

    std::string config_path;
    
    //TODO : handle config parsing
    // if (ac > 2)
    //     std::cout << "Usage: webserv [<config_file_path>]" << std::endl;
    // if (ac == 2)
    //     config_path = av[1];

    // std::ifstream test_file(config_path.c_str());
    // if (!test_file.is_open()) {
    //     std::cerr << "Configuration file not found: " << config_path << "\n";
    //     return 1;
    // }
    // test_file.close();
 
    // const int port = load_port(config_path);

    // if (port <= 0 || port > 65535) {
    //     std::cerr << "Invalid port loaded: " << port << "\n";
    //     return 1;
    // }
    // std::cout << "Using port " << port << " from config: " << config_path << "\n";

    ServerConfig server_config;
    server_config.port = 8080;
    server_config.locations.push_back(LocationConfig());
    server_config.locations.push_back(LocationConfig());
    server_config.locations.push_back(LocationConfig());
    // server_config.locations.push_back(LocationConfig());
    server_config.server_name = "localhost";
    server_config.root = "./site1/www";
    server_config.index_files.push_back("index.html");
    server_config.client_timeout = CLIENT_TIMEOUT;
    server_config.error_pages[404] = "./site1/www/errors/404.html";
    server_config.error_pages[500] = "./site1/www/errors/500.html";

    server_config.locations[0].location = "/";
    server_config.locations[0].path = "./site1/www";
    server_config.locations[0].allowed_methods.push_back("GET");

    server_config.locations[1].location = "/images";
    server_config.locations[1].path = "./site1/images";
    server_config.locations[1].allowed_methods.push_back("GET");

    server_config.locations[2].location = "/upload";
    server_config.locations[2].path = "./site1/upload";
    server_config.locations[2].allowed_methods.push_back("POST");
    



    Server server(server_config);
    g_server = &server;

    if (!server.init())
    {
        std::cerr << "Failed to init server on port " << server_config.port << std::endl;
        return (1);
    }

    std::cout << "Server init sucessfully" << std::endl;
    std::cout << "Open in your browser:" << std::endl;
    std::cout << "  -> http://127.0.0.1:" << server_config.port << "/" << std::endl;
    std::cout << "  -> http://localhost:" << server_config.port << "/" << std::endl;
    std::cout << "Serving root: " << server_config.root << std::endl;

    server.run();

    } catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return (1);
    }

    std::cout << "Server shutdown finished sucessfulluy" << std::endl;

    return 0;
}
