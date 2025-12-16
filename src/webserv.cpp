#include "macros.hpp"
#include "Config.hpp"
#include "ProcessManager.hpp"
#include "FastCgiBackend.hpp"

static std::vector<ServerConfig>	loadConfigs(const std::string &path)
{
	if (path.empty())
		throw std::runtime_error("No config file provided. Usage: webserv <config>");

	Config							cfg(path);
	const std::vector<ServerConfig>	&parsed = cfg.getServers();

	if (parsed.empty())
		throw std::runtime_error("Config did not produce any server entries");
	cfg.printDebug(std::cout);
	return std::vector<ServerConfig>(parsed.begin(), parsed.end());
}

static void	launchServers(ProcessManager &pm,
							const std::vector<ServerConfig> &configs)
{
	std::vector<ServerConfig>::const_iterator	it;
	pid_t										pid;

	std::cout << "Launching " << configs.size() << " server instance(s)."
		<< std::endl;
	for (it = configs.begin(); it != configs.end(); ++it)
	{
		pid = pm.spawnServerProcess(*it);
		pm.addProcess(pid, it->port, it->server_name);
		std::cout << "Spawned server PID " << pid << " on port "
			<< it->port << " (" << it->server_name << ")" << std::endl;
	}
}

int	main(int ac, char *av[], char **env)
{
	if (ac != 2)
	{
		std::cerr << "Usage: webserv <config_file_path>" << std::endl;
		return (1);
	}

	ProcessManager	pm;
	FastCgiBackend	fcgi;

	try
	{
		pm.installSignalHandlers();

		std::vector<ServerConfig>	configs = loadConfigs(av[1]);

		fcgi.ensureBackendsRunning(configs, env);
		launchServers(pm, configs);
		pm.monitorChildren();
	}
	catch (const std::exception &e)
	{
		if (pm.hasChildren())
		{
			pm.terminateAll();
			int	status = 0;
			while (waitpid(-1, &status, WNOHANG) > 0);
		}
		std::cerr << "Error: " << e.what() << std::endl;
		return (1);
	}
	std::cout << "All server processes stopped cleanly" << std::endl;
	return (0);
}
