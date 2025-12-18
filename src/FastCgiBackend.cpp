#include "FastCgiBackend.hpp"

FastCgiBackend::FastCgiBackend(void) : _endpoints()
{
}

FastCgiBackend::~FastCgiBackend(void)
{
}

bool	FastCgiBackend::isPortOpen(const std::string &host, int port) const
{
	int					fd;
	struct sockaddr_in	addr;
	struct timeval		tv;
	int					res;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return (false);
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(static_cast<unsigned short>(port));
	if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1)
	{
		close(fd);
		return (false);
	}
	tv.tv_sec = 0;
	tv.tv_usec = 200000;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	res = connect(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
	close(fd);
	return (res == 0);
}

void	FastCgiBackend::collectEndpoints(const std::vector<ServerConfig> &servers)
{
	size_t					i;
	size_t					j;
	std::string::size_type	colon;
	std::string				host;
	std::string				portStr;
	int						port;

	_endpoints.clear();
	for (i = 0; i < servers.size(); ++i)
	{
		const ServerConfig &s = servers[i];
		for (j = 0; j < s.locations.size(); ++j)
		{
			const LocationConfig &loc = s.locations[j];
			if (loc.fastcgi_pass.empty())
				continue;
			colon = loc.fastcgi_pass.find(':');
			if (colon == std::string::npos)
			{
				// No port specified; assume default FastCGI port 9000 when a hostname is given
				host = loc.fastcgi_pass;
				port = 9000;
			}
			else
			{
				host = loc.fastcgi_pass.substr(0, colon);
				portStr = loc.fastcgi_pass.substr(colon + 1);
				std::istringstream	iss(portStr);
				iss >> port;
				if (!iss || port <= 0)
					continue;
			}
			// Only attempt local hosts
			if (host != "127.0.0.1" && host != "localhost" && host != "0.0.0.0")
				continue;
			_endpoints.insert(std::make_pair(host, port));
		}
	}
}

void	FastCgiBackend::startBackend(const std::string &host, int port, char **env)
{
	const char		*script = "./scripts/start_fcgi_backend.sh";
	std::ostringstream	oss;
	pid_t			cpid;
	int				devnull;
	int				status;

	oss << port;
	std::string	portArg = oss.str();
	std::cout << "FastCGI backend not reachable on " << host << ":"
		<< port << ", attempting to start..." << std::endl;
	cpid = fork();
	if (cpid == 0)
	{
		devnull = open("/dev/null", O_WRONLY);
		if (devnull >= 0)
		{
			dup2(devnull, STDOUT_FILENO);
			dup2(devnull, STDERR_FILENO);
			if (devnull > STDERR_FILENO)
				close(devnull);
		}
		const char *argv_exec[] = {script, "start", portArg.c_str(), NULL};
		execve(script, const_cast<char * const *>(argv_exec), env);
		_exit(127);
	}
	else if (cpid > 0)
	{
		status = 0;
		waitpid(cpid, &status, 0);
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
			std::cout << "Started FastCGI helper for port " << port << std::endl;
		else
			std::cerr << "FastCGI helper failed for port " << port << std::endl;
	}
	else
		std::cerr << "Failed to fork for FastCGI helper" << std::endl;
}

bool	FastCgiBackend::waitForBackend(const std::string &host, int port) const
{
	int	retries;

	for (retries = 0; retries < 15; ++retries)
	{
		if (isPortOpen(host, port))
			return (true);
		usleep(200000);
	}
	return (false);
}

void	FastCgiBackend::ensureBackendsRunning(const std::vector<ServerConfig> &servers, char **env)
{
	EndpointSet::const_iterator	it;

	collectEndpoints(servers);
	for (it = _endpoints.begin(); it != _endpoints.end(); ++it)
	{
		const std::string	&host = it->first;
		int					port = it->second;

		if (isPortOpen(host, port))
		{
			std::cout << "FastCGI backend ready on " << host << ":"
				<< port << std::endl;
			continue;
		}
		startBackend(host, port, env);
		if (waitForBackend(host, port))
			std::cout << "FastCGI backend started on " << host << ":"
				<< port << std::endl;
		else
			std::cerr << "Warning: FastCGI backend not reachable on "
				<< host << ":" << port << std::endl;
	}
}
