#pragma once

#include "ext_libs.hpp"
#include "Config.hpp"

struct ServerProcess
{
	pid_t		pid;
	int			port;
	std::string	name;
};

class ProcessManager
{
public:
	ProcessManager(void);
	~ProcessManager(void);

	void	installSignalHandlers(void);
	pid_t	spawnServerProcess(const ServerConfig &config);
	void	addProcess(pid_t pid, int port, const std::string &name);
	void	terminateAll(void);
	void	monitorChildren(void);
	bool	hasChildren(void) const;

	static volatile std::sig_atomic_t	g_stopRequested;

private:
	std::vector<ServerProcess>	_children;

	void	printBanner(const ServerConfig &config) const;
	void	handleChildExit(pid_t pid, int status);

	ProcessManager(const ProcessManager &src);
	ProcessManager &operator=(const ProcessManager &rhs);
};
