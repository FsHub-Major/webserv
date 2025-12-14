#include "Config.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>

namespace ConfigUtils {
	std::string stripInlineComment(const std::string& line);
	std::vector<std::string> splitTokens(const std::string& statement);
	std::string removeTrailingSemicolon(std::string line);
	void trim(std::string& str);
	bool parseBoolToken(std::string value);
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
	location.fastcgi_pass.clear();
	location.has_return = false;
	location.return_code = 0;
	location.return_target.clear();
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

		if (current[current.size() - 1] != ';')
			throw std::runtime_error("Directive inside location block must end with ';': " + current);

		current = ConfigUtils::removeTrailingSemicolon(current);
		trim(current);
		std::vector<std::string> tokens = ConfigUtils::splitTokens(current);
		if (tokens.empty())
			continue;

		const std::string directive = tokens[0];
		if ((directive == "path" || directive == "root") && tokens.size() >= 2)
			location.path = tokens[1];
		else if (directive == "methods" || directive == "allow_methods" || directive == "allowed_methods")
		{
			if (tokens.size() < 2)
				throw std::runtime_error("methods directive requires at least one value");
			location.allowed_methods.assign(tokens.begin() + 1, tokens.end());
		}
		else if (directive == "autoindex" && tokens.size() >= 2)
			location.autoindex = ConfigUtils::parseBoolToken(tokens[1]);
		else if ((directive == "upload_store" || directive == "upload_dir") && tokens.size() >= 2)
			location.upload_dir = tokens[1];
		else if ((directive == "cgi_extension" || directive == "cgi_extensions") && tokens.size() >= 2)
			location.cgi_extensions.assign(tokens.begin() + 1, tokens.end());
		else if (directive == "cgi_path" && tokens.size() >= 2)
			location.cgi_path = tokens[1];
		else if (directive == "fastcgi_pass" && tokens.size() >= 2)
			location.fastcgi_pass = tokens[1];
		else if (directive == "return" && tokens.size() >= 3)
		{
			location.has_return = true;
			location.return_code = std::atoi(tokens[1].c_str());
			location.return_target = tokens[2];
		}
		else
			throw std::runtime_error("Unknown location directive: " + directive);
	}

	if (!found_closing_brace)
		throw std::runtime_error("Unclosed location block (missing closing brace)");
	if (location.allowed_methods.empty())
		location.allowed_methods.push_back("GET");

	return location;
}
