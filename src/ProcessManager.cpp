#include "ProcessManager.hpp"
#include "Server.hpp"

volatile std::sig_atomic_t ProcessManager::g_stopRequested = 0;

static void	signalHandler(int sig)
{
	(void)sig;
	ProcessManager::g_stopRequested = 1;
}

ProcessManager::ProcessManager(void) : _children()
{
}

ProcessManager::~ProcessManager(void)
{
	if (!_children.empty())
		terminateAll();
}

void	ProcessManager::installSignalHandlers(void)
{
	std::signal(SIGINT, signalHandler);
	std::signal(SIGTERM, signalHandler);
}

void	ProcessManager::printBanner(const ServerConfig &config) const
{
	std::cout << "Server initialized successfully" << std::endl;
	std::cout << "Open in your browser:" << std::endl;
	std::cout << "  -> http://127.0.0.1:" << config.port << "/" << std::endl;
	std::cout << "  -> http://localhost:" << config.port << "/" << std::endl;
	std::cout << "Serving root: " << config.root << std::endl;
}

pid_t	ProcessManager::spawnServerProcess(const ServerConfig &config)
{
	pid_t	pid;

	pid = fork();
	if (pid < 0)
		throw std::runtime_error(std::string("fork() failed: ") + std::strerror(errno));
	if (pid == 0)
	{
		Server	server(config);

		if (!server.init())
		{
			std::cerr << "Failed to init server on port " << config.port << std::endl;
			_exit(EXIT_FAILURE);
		}
		printBanner(config);
		server.run();
		std::cout << "Server shutdown (port " << config.port << ")" << std::endl;
		_exit(EXIT_SUCCESS);
	}
	return (pid);
}

void	ProcessManager::addProcess(pid_t pid, int port, const std::string &name)
{
	ServerProcess	proc;

	proc.pid = pid;
	proc.port = port;
	proc.name = name;
	_children.push_back(proc);
}

void	ProcessManager::terminateAll(void)
{
	std::vector<ServerProcess>::iterator	it;

	for (it = _children.begin(); it != _children.end(); ++it)
	{
		if (it->pid > 0)
			kill(it->pid, SIGTERM);
	}
}

bool	ProcessManager::hasChildren(void) const
{
	return (!_children.empty());
}

void	ProcessManager::handleChildExit(pid_t pid, int status)
{
	std::vector<ServerProcess>::iterator	it;

	for (it = _children.begin(); it != _children.end(); ++it)
	{
		if (it->pid != pid)
			continue;
		if (WIFEXITED(status))
		{
			std::cout << "Server PID " << pid << " (" << it->name << ":"
				<< it->port << ") exited with status "
				<< WEXITSTATUS(status) << std::endl;
		}
		else if (WIFSIGNALED(status))
		{
			std::cout << "Server PID " << pid << " (" << it->name << ":"
				<< it->port << ") terminated by signal "
				<< WTERMSIG(status) << std::endl;
		}
		else
		{
			std::cout << "Server PID " << pid << " (" << it->name << ":"
				<< it->port << ") stopped" << std::endl;
		}
		_children.erase(it);
		return ;
	}
}

void	ProcessManager::monitorChildren(void)
{
	bool	termBroadcast;
	int		status;
	pid_t	pid;

	termBroadcast = false;
	while (!_children.empty())
	{
		if (g_stopRequested && !termBroadcast)
		{
			std::cout << "\nTermination requested, stopping servers..." << std::endl;
			terminateAll();
			termBroadcast = true;
		}
		status = 0;
		pid = waitpid(-1, &status, 0);
		if (pid < 0)
		{
			if (errno == EINTR)
				continue;
			perror("waitpid");
			break;
		}
		handleChildExit(pid, status);
	}
}
