#include "Config.hpp"
#include "macros.hpp"
#include "webserv.hpp"

namespace ConfigUtils {
	std::string stripInlineComment(const std::string& line);
	bool endsWithBrace(const std::string& line);
	std::vector<std::string> splitTokens(const std::string& statement);
	std::string removeTrailingSemicolon(std::string line);
	void trim(std::string& str);
	size_t parseSizeToken(const std::string& token);
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
	defaults.root = ""; // Don't provide default root - must be explicit
	defaults.index_files.clear();
	defaults.index_files.push_back("index.html");
	defaults.error_pages.clear();
	defaults.client_max_body_size = 1024 * 1024; // 1MB
	defaults.client_timeout = CLIENT_TIMEOUT;
	defaults.locations.clear();

	std::string raw_line;
	while (std::getline(file, raw_line))
	{
		std::string line = ConfigUtils::stripInlineComment(raw_line);
		trim(line);
		if (line.empty())
			continue;

		if (line.compare(0, 6, "server") == 0)
		{
			if (!ConfigUtils::endsWithBrace(line))
			{
				std::string brace_line;
				while (std::getline(file, brace_line))
				{
					brace_line = ConfigUtils::stripInlineComment(brace_line);
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

		line = ConfigUtils::removeTrailingSemicolon(line);
		trim(line);
		const std::vector<std::string> tokens = ConfigUtils::splitTokens(line);
		if (tokens.empty())
			continue;

		const std::string directive = tokens[0];
		if (directive == "max_client_body_size" && tokens.size() >= 2)
			defaults.client_max_body_size = ConfigUtils::parseSizeToken(tokens[1]);
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
