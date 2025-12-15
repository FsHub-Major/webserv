// Minimal HTTP server in C++ using BSD sockets
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

volatile std::sig_atomic_t g_stopRequested = 0;

void handleTerminationSignal(int)
{
    g_stopRequested = 1;
}

void installSignalHandlers()
{
    // Use the simple signal() API which is portable and avoids needing
    // to rely on struct sigaction layout in different environments.
    std::signal(SIGINT, handleTerminationSignal);
    std::signal(SIGTERM, handleTerminationSignal);
}

std::vector<ServerConfig> loadServerConfigs(const std::string &config_path)
{
    if (config_path.empty())
    {
        throw std::runtime_error("No configuration file provided. \
            \nUsage: webserv <config_file_path>");
    }

    Config config(config_path);
    const std::vector<ServerConfig> &parsed_servers = config.getServers();
    if (parsed_servers.empty())
        throw std::runtime_error("Config did not produce any server entries");

    config.printDebug(std::cout);
    std::vector<ServerConfig> servers(parsed_servers.begin(), parsed_servers.end());
    return servers;
}

// Try to connect to host:port (returns true if connect succeeds)
static bool isPortOpen(const std::string &host, int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return false;
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(port));
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
    {
        close(fd);
        return false;
    }
    // set a short timeout for connect
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 200000; // 200ms
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    int res = connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
    if (res == 0) { close(fd); return true; }
    close(fd);
    return false;
}

// Ensure local FastCGI backends are running for any fastcgi_pass entries that point to localhost.
static void ensureFastCgiBackends(const std::vector<ServerConfig> &servers)
{
    std::set<std::pair<std::string,int> > endpoints;
    for (size_t i = 0; i < servers.size(); ++i)
    {
        const ServerConfig &s = servers[i];
        for (size_t j = 0; j < s.locations.size(); ++j)
        {
            const LocationConfig &loc = s.locations[j];
            if (loc.fastcgi_pass.empty()) continue;
            std::string::size_type colon = loc.fastcgi_pass.find(':');
            if (colon == std::string::npos) continue;
            std::string host = loc.fastcgi_pass.substr(0, colon);
            std::string portStr = loc.fastcgi_pass.substr(colon+1);
            int port = 0;
            std::istringstream iss(portStr);
            iss >> port;
            if (!iss || port <= 0) continue;
            // only attempt local hosts
            if (host != "127.0.0.1" && host != "localhost" && host != "0.0.0.0") continue;
            endpoints.insert(std::make_pair(host, port));
        }
    }

    for (std::set<std::pair<std::string,int> >::const_iterator it = endpoints.begin(); it != endpoints.end(); ++it)
    {
        const std::string &host = it->first;
        int port = it->second;
        if (isPortOpen(host, port))
        {
            std::cout << "FastCGI backend already listening on " << host << ":" << port << std::endl;
            continue;
        }

        // Try to start the helper script if present
        const char *script = "./scripts/start_fcgi_backend.sh";
        std::ostringstream pss; pss << port; std::string portArg = pss.str();
        std::cout << "FastCGI backend not reachable on " << host << ":" << port << ", attempting to start via " << script << std::endl;

        // Use fork
        pid_t cpid = fork();
        if (cpid == 0)
        {
            // child
            int devnull = open("/dev/null", O_WRONLY);
            if (devnull >= 0)
            {
                dup2(devnull, STDOUT_FILENO);
                dup2(devnull, STDERR_FILENO);
                if (devnull > STDERR_FILENO)
                    close(devnull);
            }
            const char *argv_exec[] = { script, "start", portArg.c_str(), NULL };
            execv(script, const_cast<char* const*>(argv_exec));
            // If exec fails
            _exit(127);
        }
        else if (cpid > 0)
        {
            int status = 0;
            waitpid(cpid, &status, 0);
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                std::cout << "Started FastCGI helper (exit 0) for port " << port << std::endl;
            } else {
                std::cerr << "FastCGI helper failed for port " << port << " (exit=" << (WIFEXITED(status) ? WEXITSTATUS(status) : -1) << ")" << std::endl;
            }
        }
        else
        {
            // fork failed
            std::cerr << "Failed to fork to start FastCGI helper" << std::endl;
        }

        // Wait up to ~3s for backend to start
        bool ok = false;
        for (int retries = 0; retries < 15; ++retries)
        {
            if (isPortOpen(host, port)) { ok = true; break; }
            usleep(200000); // 200ms
        }
        if (ok)
            std::cout << "FastCGI backend started on " << host << ":" << port << std::endl;
        else
            std::cerr << "Warning: FastCGI backend still not reachable on " << host << ":" << port << std::endl;
    }
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
    if (ac != 2)
    {
        std::cerr << "Usage: webserv <config_file_path>" << std::endl;
        return 1;
    }

    const std::string config_path = av[1];
    std::vector<ServerProcess> children;

    try
    {
        installSignalHandlers();

        const std::vector<ServerConfig> server_configs = loadServerConfigs(config_path);
        // Ensure any local FastCGI backends referenced by config are running
        ensureFastCgiBackends(server_configs);
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
