#include "Config.hpp"

namespace ConfigUtils
{
	std::string stripInlineComment(const std::string& line);
	std::vector<std::string> splitTokens(const std::string& statement);
	std::string removeTrailingSemicolon(std::string line);
	void trim(std::string& str);
	size_t parseSizeToken(const std::string& token);
	int parsePortToken(const std::string& token);
}

ServerConfig Config::parseServerBlock(std::ifstream& file, std::string& line, const ServerConfig& defaults)
{
	(void)line;
	ServerConfig server = defaults;
	server.locations.clear();
	// Keep root from defaults if set globally
	bool has_listen = false;
	bool has_directives = false;
	bool found_closing_brace = false;

	std::string raw_line;
	while (std::getline(file, raw_line))
	{
		std::string current = ConfigUtils::stripInlineComment(raw_line);
		trim(current);
		if (current.empty())
			continue;

		if (current == "}")
		{
			found_closing_brace = true;
			break;
		}

		if (current.compare(0, 8, "location") == 0)
		{
			std::string header = current;
			if (header.find('{') == std::string::npos)
			{
				std::string brace_line;
				while (std::getline(file, brace_line))
				{
					brace_line = ConfigUtils::stripInlineComment(brace_line);
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
			std::vector<std::string> tokens = ConfigUtils::splitTokens(before_brace);
			if (tokens.size() < 2)
				throw std::runtime_error("Location block requires a path");

			const std::string location_path = tokens[1];
			LocationConfig location = parseLocationBlock(file, location_path);
			server.locations.push_back(location);
			continue;
		}

		if (current[current.size() - 1] != ';')
			throw std::runtime_error("Directive inside server block must end with ';': " + current);

		current = ConfigUtils::removeTrailingSemicolon(current);
		trim(current);
		std::vector<std::string> tokens = ConfigUtils::splitTokens(current);
		if (tokens.empty())
			continue;

		const std::string directive = tokens[0];
		if (directive == "listen")
		{
			if (tokens.size() < 2)
				throw std::runtime_error("listen directive requires a port value");
			if (has_listen)
				throw std::runtime_error("Duplicate listen directive in server block");
			has_listen = true;
			has_directives = true;
			server.port = ConfigUtils::parsePortToken(tokens[1]);
		}
		else if (directive == "server_name")
		{
			if (tokens.size() < 2)
				throw std::runtime_error("server_name directive requires a value");
			has_directives = true;
			server.server_name = tokens[1];
		}
		else if (directive == "root")
		{
			if (tokens.size() < 2)
				throw std::runtime_error("root directive requires a value");
			has_directives = true;
			server.root = tokens[1];
		}
		else if (directive == "index")
		{
			if (tokens.size() < 2)
				throw std::runtime_error("index directive requires at least one value");
			has_directives = true;
			server.index_files.assign(tokens.begin() + 1, tokens.end());
		}
		else if (directive == "error_page")
		{
			if (tokens.size() < 3)
				throw std::runtime_error("error_page directive requires error code and path");
			has_directives = true;
			server.error_pages[std::atoi(tokens[1].c_str())] = tokens[2];
		}
		else if (directive == "client_max_body_size")
		{
			if (tokens.size() < 2)
				throw std::runtime_error("client_max_body_size directive requires a value");
			has_directives = true;
			server.client_max_body_size = ConfigUtils::parseSizeToken(tokens[1]);
		}
		else if (directive == "client_timeout")
		{
			if (tokens.size() < 2)
				throw std::runtime_error("client_timeout directive requires a value");
			has_directives = true;
			server.client_timeout = std::atoi(tokens[1].c_str());
		}
		else if (directive == "cgi_extension" || directive == "cgi_extensions")
		{
			// CGI extensions at server level - store for later use if needed
			// For now just mark as valid directive
			has_directives = true;
		}
		else
			throw std::runtime_error("Unknown server directive: " + directive);
	}

	if (!found_closing_brace)
		throw std::runtime_error("Unclosed server block (missing closing brace)");
	if (!has_directives)
		throw std::runtime_error("Empty server block (no directives)");
	if (server.root.empty())
		throw std::runtime_error("Server block missing root directive (must be set globally or per-server)");
	return server;
}
