

#include "Config.hpp"
#include "ext_libs.hpp"
#include "macros.hpp"
#include "webserv.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

std::string stripInlineComment(const std::string& line)
{
	bool in_single_quote = false;
	bool in_double_quote = false;

	for (size_t i = 0; i < line.size(); ++i)
	{
		const char c = line[i];
		if (c == '\'' && !in_double_quote)
			in_single_quote = !in_single_quote;
		else if (c == '"' && !in_single_quote)
			in_double_quote = !in_double_quote;
		else if (!in_single_quote && !in_double_quote)
		{
			if (c == '#')
				return line.substr(0, i);
			if (c == '/' && i + 1 < line.size() && line[i + 1] == '/')
				return line.substr(0, i);
		}
	}
	return line;
}

bool endsWithBrace(const std::string& line)
{
	for (std::string::const_reverse_iterator it = line.rbegin(); it != line.rend(); ++it)
	{
		if (!std::isspace(static_cast<unsigned char>(*it)))
			return *it == '{';
	}
	return false;
}

void toLower(std::string& value)
{
	std::transform(value.begin(), value.end(), value.begin(), ::tolower);
}

bool parseBoolToken(std::string value)
{
	toLower(value);
	return (value == "on" || value == "true" || value == "1");
}

size_t parseSizeToken(const std::string& token)
{
	if (token.empty())
		return 0;

	char suffix = token[token.size() - 1];
	size_t multiplier = 1;
	std::string number = token;

	if (!std::isdigit(static_cast<unsigned char>(suffix)))
	{
		if (suffix == 'k' || suffix == 'K')
			multiplier = 1024;
		else if (suffix == 'm' || suffix == 'M')
			multiplier = 1024 * 1024;
		else if (suffix == 'g' || suffix == 'G')
			multiplier = 1024 * 1024 * 1024ULL;
		else
			throw std::runtime_error("Unknown size suffix: " + token);
		number = token.substr(0, token.size() - 1);
	}

	std::istringstream iss(number);
	size_t value = 0;
	if (!(iss >> value))
		throw std::runtime_error("Invalid numeric value: " + token);
	return value * multiplier;
}

int parsePortToken(const std::string& token)
{
	std::string port_part = token;
	const size_t colon = token.find(':');
	if (colon != std::string::npos)
		port_part = token.substr(colon + 1);

	std::istringstream iss(port_part);
	int port = 0;
	if (!(iss >> port) || port <= 0 || port > 65535)
		throw std::runtime_error("Invalid port value: " + token);
	return port;
}

std::vector<std::string> splitTokens(const std::string& statement)
{
	std::vector<std::string> tokens;
	std::istringstream iss(statement);
	std::string token;
	while (iss >> token)
		tokens.push_back(token);
	return tokens;
}

std::string removeTrailingSemicolon(std::string line)
{
	if (!line.empty() && line[line.size() - 1] == ';')
		line.erase(line.size() - 1);
	return line;
}

} // namespace

Config::Config(const std::string& config_path)
{
	if (config_path.empty())
		throw std::runtime_error("Config path cannot be empty");
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

void Config::printDebug(std::ostream& os) const
{
	os << "\n=== Parsed Servers: " << servers.size() << " ===" << std::endl;
	for (size_t i = 0; i < servers.size(); ++i)
	{
		const ServerConfig& server = servers[i];
		os << "Server #" << (i + 1) << std::endl;
		os << "  Port: " << server.port << std::endl;
		os << "  Server Name: " << server.server_name << std::endl;
		os << "  Root: " << server.root << std::endl;
		os << "  Client Max Body Size: " << server.client_max_body_size << std::endl;
		os << "  Client Timeout: " << server.client_timeout << std::endl;

		os << "  Index Files: ";
		if (server.index_files.empty())
			os << "(none)";
		else
		{
			for (size_t idx = 0; idx < server.index_files.size(); ++idx)
			{
				if (idx)
					os << ", ";
				os << server.index_files[idx];
			}
		}
		os << std::endl;

		if (server.error_pages.empty())
			os << "  Error Pages: (none)" << std::endl;
		else
		{
			os << "  Error Pages:" << std::endl;
			for (std::map<int, std::string>::const_iterator ep = server.error_pages.begin();
				 ep != server.error_pages.end(); ++ep)
			{
				os << "    " << ep->first << " -> " << ep->second << std::endl;
			}
		}

		os << "  Locations: " << server.locations.size() << std::endl;
		for (size_t locIdx = 0; locIdx < server.locations.size(); ++locIdx)
		{
			const LocationConfig& loc = server.locations[locIdx];
			os << "    Location #" << (locIdx + 1) << ": " << loc.location << std::endl;
			os << "      Path: " << loc.path << std::endl;
			os << "      Methods: ";
			if (loc.allowed_methods.empty())
				os << "(none)";
			else
			{
				for (size_t m = 0; m < loc.allowed_methods.size(); ++m)
				{
					if (m)
						os << ", ";
					os << loc.allowed_methods[m];
				}
			}
			os << std::endl;
			os << "      Autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;
			os << "      Upload Dir: " << (loc.upload_dir.empty() ? "(none)" : loc.upload_dir) << std::endl;
			os << "      CGI Path: " << (loc.cgi_path.empty() ? "(none)" : loc.cgi_path) << std::endl;
			os << "      CGI Extensions: ";
			if (loc.cgi_extensions.empty())
				os << "(none)";
			else
			{
				for (size_t ce = 0; ce < loc.cgi_extensions.size(); ++ce)
				{
					if (ce)
						os << ", ";
					os << loc.cgi_extensions[ce];
				}
			}
			os << std::endl;

			if (loc.has_return)
				os << "      Redirect: " << loc.return_code << " -> " << loc.return_target << std::endl;
			else
				os << "      Redirect: (none)" << std::endl;
		}

		if (i + 1 != servers.size())
			os << std::endl;
	}
	os << std::endl;
}

void Config::parseConfigFile(const std::string& path)
{
	std::ifstream file(path.c_str());
	if (!file.is_open())
		throw std::runtime_error("Unable to open config file: " + path);

	servers.clear();

	ServerConfig defaults;
	defaults.port = 8080;
	defaults.server_name = "localhost";
	defaults.root = "./site1/www";
	defaults.index_files.clear();
	defaults.index_files.push_back("index.html");
	defaults.error_pages.clear();
	defaults.client_max_body_size = 1024 * 1024; // 1MB
	defaults.client_timeout = CLIENT_TIMEOUT;
	defaults.locations.clear();

	std::string raw_line;
	while (std::getline(file, raw_line))
	{
		std::string line = stripInlineComment(raw_line);
		trim(line);
		if (line.empty())
			continue;

		if (line.compare(0, 6, "server") == 0)
		{
			if (!endsWithBrace(line))
			{
				std::string brace_line;
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
			ServerConfig server = parseServerBlock(file, line, defaults);
			servers.push_back(server);
			continue;
		}

		if (!line.empty() && line[line.size() - 1] != ';')
			throw std::runtime_error("Directive outside server block must end with ';': " + line);

		line = removeTrailingSemicolon(line);
		trim(line);
		const std::vector<std::string> tokens = splitTokens(line);
		if (tokens.empty())
			continue;

		const std::string directive = tokens[0];
		if (directive == "max_client_body_size" && tokens.size() >= 2)
			defaults.client_max_body_size = parseSizeToken(tokens[1]);
		else if (directive == "client_timeout" && tokens.size() >= 2)
			defaults.client_timeout = std::atoi(tokens[1].c_str());
		else if (directive == "error_page" && tokens.size() >= 3)
			defaults.error_pages[std::atoi(tokens[1].c_str())] = tokens[2];
		else if (directive == "root" && tokens.size() >= 2)
			defaults.root = tokens[1];
		else if (directive == "server_name" && tokens.size() >= 2)
			defaults.server_name = tokens[1];
		else if (directive == "index" && tokens.size() >= 2)
			defaults.index_files.assign(tokens.begin() + 1, tokens.end());
		else
			std::cerr << "Warning: Unknown global directive '" << directive << "'" << std::endl;
	}

	if (servers.empty())
		throw std::runtime_error("Config file does not define any server blocks");
}

ServerConfig Config::parseServerBlock(std::ifstream& file, std::string& line, const ServerConfig& defaults)
{
	(void)line;
	ServerConfig server = defaults;
	server.locations.clear();

	std::string raw_line;
	while (std::getline(file, raw_line))
	{
		std::string current = stripInlineComment(raw_line);
		trim(current);
		if (current.empty())
			continue;

		if (current == "}")
			break;

		if (current.compare(0, 8, "location") == 0)
		{
			std::string header = current;
			if (header.find('{') == std::string::npos)
			{
				std::string brace_line;
				while (std::getline(file, brace_line))
				{
					brace_line = stripInlineComment(brace_line);
					trim(brace_line);
					if (brace_line.empty())
						continue;
					header += " " + brace_line;
					if (brace_line.find('{') != std::string::npos)
						break;
				}
			}

			std::string before_brace = header.substr(0, header.find('{'));
			trim(before_brace);
			std::vector<std::string> tokens = splitTokens(before_brace);
			if (tokens.size() < 2)
				throw std::runtime_error("Location block requires a path");

			const std::string location_path = tokens[1];
			LocationConfig location = parseLocationBlock(file, location_path);
			server.locations.push_back(location);
			continue;
		}

		if (current[current.size() - 1] != ';')
			throw std::runtime_error("Directive inside server block must end with ';': " + current);

		current = removeTrailingSemicolon(current);
		trim(current);
		std::vector<std::string> tokens = splitTokens(current);
		if (tokens.empty())
			continue;

		const std::string directive = tokens[0];
		if (directive == "listen" && tokens.size() >= 2)
			server.port = parsePortToken(tokens[1]);
		else if (directive == "server_name" && tokens.size() >= 2)
			server.server_name = tokens[1];
		else if (directive == "root" && tokens.size() >= 2)
			server.root = tokens[1];
		else if (directive == "index" && tokens.size() >= 2)
			server.index_files.assign(tokens.begin() + 1, tokens.end());
		else if (directive == "error_page" && tokens.size() >= 3)
			server.error_pages[std::atoi(tokens[1].c_str())] = tokens[2];
		else if (directive == "client_max_body_size" && tokens.size() >= 2)
			server.client_max_body_size = parseSizeToken(tokens[1]);
		else if (directive == "client_timeout" && tokens.size() >= 2)
			server.client_timeout = std::atoi(tokens[1].c_str());
		else
			std::cerr << "Warning: Unknown server directive '" << directive << "'" << std::endl;
	}

	if (server.root.empty())
		throw std::runtime_error("Server block missing root directive");
	return server;
}

LocationConfig Config::parseLocationBlock(std::ifstream& file, const std::string& location_path)
{
	LocationConfig location;
	location.location = location_path;
	location.path = location_path;
	location.allowed_methods.clear();
	location.autoindex = false;
	location.upload_dir.clear();
	location.cgi_extensions.clear();
	location.cgi_path.clear();
	location.has_return = false;
	location.return_code = 0;
	location.return_target.clear();

	std::string raw_line;
	while (std::getline(file, raw_line))
	{
		std::string current = stripInlineComment(raw_line);
		trim(current);
		if (current.empty())
			continue;

		if (current == "}")
			break;

		if (current[current.size() - 1] != ';')
			throw std::runtime_error("Directive inside location block must end with ';': " + current);

		current = removeTrailingSemicolon(current);
		trim(current);
		std::vector<std::string> tokens = splitTokens(current);
		if (tokens.empty())
			continue;

		const std::string directive = tokens[0];
		if ((directive == "path" || directive == "root") && tokens.size() >= 2)
			location.path = tokens[1];
		else if (directive == "methods" && tokens.size() >= 2)
			location.allowed_methods.assign(tokens.begin() + 1, tokens.end());
		else if (directive == "autoindex" && tokens.size() >= 2)
			location.autoindex = parseBoolToken(tokens[1]);
		else if ((directive == "upload_store" || directive == "upload_dir") && tokens.size() >= 2)
			location.upload_dir = tokens[1];
		else if ((directive == "cgi_extension" || directive == "cgi_extensions") && tokens.size() >= 2)
			location.cgi_extensions.assign(tokens.begin() + 1, tokens.end());
		else if (directive == "cgi_path" && tokens.size() >= 2)
			location.cgi_path = tokens[1];
		else if (directive == "return" && tokens.size() >= 3)
		{
			location.has_return = true;
			location.return_code = std::atoi(tokens[1].c_str());
			location.return_target = tokens[2];
		}
		else
			std::cerr << "Warning: Unknown location directive '" << directive << "'" << std::endl;
	}

	if (location.allowed_methods.empty())
		location.allowed_methods.push_back("GET");

	return location;
}

std::vector<std::string> Config::split(const std::string& str, char delimiter)
{
	std::vector<std::string> tokens;
	std::string current;
	for (size_t i = 0; i < str.size(); ++i)
	{
		if (str[i] == delimiter)
		{
			if (!current.empty())
			{
				tokens.push_back(current);
				current.clear();
			}
		}
		else
			current.push_back(str[i]);
	}
	if (!current.empty())
		tokens.push_back(current);
	return tokens;
}

void Config::trim(std::string& str)
{
	const std::string whitespace = " \t\r\n";
	const size_t begin = str.find_first_not_of(whitespace);
	if (begin == std::string::npos)
	{
		str.clear();
		return;
	}
	const size_t end = str.find_last_not_of(whitespace);
	str = str.substr(begin, end - begin + 1);
}