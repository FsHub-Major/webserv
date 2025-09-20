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
    server_config.server_name = "localhost";
    server_config.root = "./www";
    server_config.index_files.push_back("index.html");
    server_config.client_timeout = CLIENT_TIMEOUT;
    server_config.client_max_body_size = 1024 * 1024;  // 1MB


    Server server(server_config);
    g_server = &server;

    if (!server.init())
    {
        std::cerr << "Failed to init server on port " << server_config.port << std::endl;
        return (1);
    }

    std::cout << "Server init sucessfully" << std::endl;

    server.run();

    } catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return (1);
    }

    std::cout << "Server shutdown finished sucessfulluy" << std::endl;

    return 0;
}
