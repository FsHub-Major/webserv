#pragma once

#include "ext_libs.hpp"
#include "Config.hpp"

class FastCgiBackend
{
public:
	FastCgiBackend(void);
	~FastCgiBackend(void);

	void	ensureBackendsRunning(const std::vector<ServerConfig> &servers, char **env);

private:
	typedef std::pair<std::string, int>	Endpoint;
	typedef std::set<Endpoint>			EndpointSet;

	EndpointSet	_endpoints;

	void	collectEndpoints(const std::vector<ServerConfig> &servers);
	bool	isPortOpen(const std::string &host, int port) const;
	void	startBackend(const std::string &host, int port, char **env);
	bool	waitForBackend(const std::string &host, int port) const;

	FastCgiBackend(const FastCgiBackend &src);
	FastCgiBackend &operator=(const FastCgiBackend &rhs);
};
