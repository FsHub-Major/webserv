#include "Config.hpp"
#include "ext_libs.hpp"
#include "macros.hpp"
#include "webserv.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>

Config::Config(const std::string& config_path)
{
	if (config_path.empty())
		throw std::runtime_error("Config path cannot be empty !");
	parseConfigFile(config_path);
}

const ServerConfig* Config::getServerByPort(int port) const
{
	for (size_t i = 0; i < servers.size(); ++i)
	{
		if (servers[i].port == port)
			return &servers[i];
	}
	return NULL;
}

void Config::parseConfigFile(const std::string& path)
{
	ServerConfig	global_config;
	ServerConfig	server_config;
	std::string		raw_line;
	std::string		brace_line;
	std::string		line;
	std::ifstream	file(path.c_str());

	if (!file.is_open())
		throw std::runtime_error("Unable to open config file -> " + path);

	servers.clear();

	while (std::getline(file, raw_line))
	{
		line = stripInlineComment(raw_line);
		trim(line);
		if (line.empty())
			continue;

		if (line.compare(0, 6, "server") == 0)
		{
			if (!endsWithBrace(line))
			{
				while (std::getline(file, brace_line))
				{
					brace_line = stripInlineComment(brace_line);
					trim(brace_line);
					if (brace_line.empty())
						continue;
					if (brace_line.find('{') != std::string::npos)
						break;
				}
			}
			server_config = parseServerBlock(file, global_config);
			servers.push_back(server_config);
			continue;
		}

		if (!line.empty() && line[line.size() - 1] != ';')
			throw std::runtime_error(
				"Directive outside server block must end with ';' -> " + line);

		line = removeTrailingSemicolon(line);
		trim(line);
		const std::vector<std::string> tokens = splitTokens(line);
		if (tokens.empty())
			continue;

		parseGlobalDirective(global_config, tokens);
	}

	if (servers.empty())
		throw std::runtime_error(
			"Config file does not define any server blocks !");
}

void Config::parseGlobalDirective(ServerConfig& server_config,
                                  const std::vector<std::string>& tokens)
{
	const std::string directive = tokens[0];

	if (directive == "max_client_body_size" && tokens.size() >= 2)
		server_config.client_max_body_size = parseSizeToken(tokens[1]);
	else if (directive == "client_timeout" && tokens.size() >= 2)
		server_config.client_timeout = std::atoi(tokens[1].c_str());
	else if (directive == "error_page" && tokens.size() >= 3)
		server_config.error_pages[std::atoi(tokens[1].c_str())] = tokens[2];
	else if (directive == "root" && tokens.size() >= 2)
		server_config.root = tokens[1];
	else if (directive == "server_name" && tokens.size() >= 2)
		server_config.server_name = tokens[1];
	else if (directive == "index" && tokens.size() >= 2)
		server_config.index_files.assign(tokens.begin() + 1, tokens.end());
	else
		std::cerr << "Warning: Unknown global directive '" << directive 
		          << "'" << std::endl;
}

