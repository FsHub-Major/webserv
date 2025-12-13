// Minimal HTTP server in C++ using BSD sockets

#include <iostream>
#include <stdexcept>
#include <vector>
#include <csignal>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>

#include "macros.hpp"
#include "Config.hpp"
#include "Server.hpp"

namespace {

struct ServerProcess
{
    pid_t pid;
    int port;
    std::string name;
};

void printStartupBanner(const ServerConfig &config)
{
    std::cout << "Server initialized successfully" << std::endl;
    std::cout << "Open in your browser:" << std::endl;
    std::cout << "  -> http://127.0.0.1:" << config.port << "/" << std::endl;
    std::cout << "  -> http://localhost:" << config.port << "/" << std::endl;
    std::cout << "Serving root: " << config.root << std::endl;
}

volatile sig_atomic_t g_stopRequested = 0;

void handleTerminationSignal(int)
{
    g_stopRequested = 1;
}

void installSignalHandlers()
{
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handleTerminationSignal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

std::vector<ServerConfig> loadServerConfigs(const std::string &config_path)
{
    if (config_path.empty())
        throw std::runtime_error("no config file passed !");
    Config config(config_path);
    const std::vector<ServerConfig> &parsed_servers = config.getServers();
    if (parsed_servers.empty())
        throw std::runtime_error("Config did not produce any server entries");
    config.printDebug(std::cout);
    std::vector<ServerConfig> servers(parsed_servers.begin(), parsed_servers.end());
    return servers;
}

pid_t spawnServerProcess(const ServerConfig &server_config)
{
    pid_t pid = fork();
    if (pid < 0)
        throw std::runtime_error(std::string("fork() failed: ") + std::strerror(errno));

    if (pid == 0)
    {
        Server server(server_config);
        if (!server.init())
        {
            std::cerr << "Failed to init server on port " << server_config.port << std::endl;
            _exit(EXIT_FAILURE);
        }

        printStartupBanner(server_config);
        server.run();
        std::cout << "Server shutdown finished successfully (port " << server_config.port << ")" << std::endl;
        _exit(EXIT_SUCCESS);
    }

    return pid;
}

void terminateChildren(const std::vector<ServerProcess> &children)
{
    for (std::vector<ServerProcess>::const_iterator it = children.begin(); it != children.end(); ++it)
    {
        if (it->pid > 0)
            kill(it->pid, SIGTERM);
    }
}

void monitorChildren(std::vector<ServerProcess> &children)
{
    bool terminationBroadcast = false;

    while (!children.empty())
    {
        if (g_stopRequested && !terminationBroadcast)
        {
            std::cout << "\nTermination requested, stopping all servers..." << std::endl;
            terminateChildren(children);
            terminationBroadcast = true;
        }

        int status = 0;
        pid_t pid = waitpid(-1, &status, 0);
        if (pid < 0)
        {
            if (errno == EINTR)
                continue;
            perror("waitpid");
            break;
        }

        for (std::vector<ServerProcess>::iterator it = children.begin(); it != children.end(); ++it)
        {
            if (it->pid != pid)
                continue;

            if (WIFEXITED(status))
            {
                std::cout << "Server PID " << pid << " (" << it->name << ":" << it->port
                          << ") exited with status " << WEXITSTATUS(status) << std::endl;
            }
            else if (WIFSIGNALED(status))
            {
                std::cout << "Server PID " << pid << " (" << it->name << ":" << it->port
                          << ") terminated by signal " << WTERMSIG(status) << std::endl;
            }
            else
            {
                std::cout << "Server PID " << pid << " (" << it->name << ":" << it->port
                          << ") stopped" << std::endl;
            }

            children.erase(it);
            break;
        }
    }
}

} // namespace

int main(int ac, char *av[]) {
    if (ac > 2)
    {
        std::cerr << "Usage: webserv [<config_file_path>]" << std::endl;
        return 1;
    }

    const std::string config_path = (ac == 2) ? av[1] : "";
    std::vector<ServerProcess> children;

    try
    {
        installSignalHandlers();

        const std::vector<ServerConfig> server_configs = loadServerConfigs(config_path);
        std::cout << "Launching " << server_configs.size() << " server instance(s)." << std::endl;

        for (std::vector<ServerConfig>::const_iterator it = server_configs.begin(); it != server_configs.end(); ++it)
        {
            pid_t pid = spawnServerProcess(*it);
            ServerProcess process;
            process.pid = pid;
            process.port = it->port;
            process.name = it->server_name;
            children.push_back(process);

            std::cout << "Spawned server PID " << pid << " on port " << process.port
                      << " (" << process.name << ")" << std::endl;
        }

        monitorChildren(children);
    }
    catch (const std::exception& e)
    {
        if (!children.empty())
        {
            terminateChildren(children);
            int status = 0;
            while (waitpid(-1, &status, WNOHANG) > 0) {}
        }
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "All server processes stopped cleanly" << std::endl;
    return 0;
}
