#include "Config.hpp"
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cctype>

namespace {

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

bool endsWithBrace(const std::string& line)
{
	for (std::string::const_reverse_iterator it = line.rbegin(); 
	     it != line.rend(); ++it)
	{
		if (!std::isspace(static_cast<unsigned char>(*it)))
			return *it == '{';
	}
	return false;
}

bool isKnownServerDirective(const std::string& directive)
{
	static const std::string known[] = {
		"listen", "server_name", "root", "index", "error_page",
		"client_max_body_size", "client_timeout", "cgi_extension", 
		"fastcgi_pass"
	};
	for (size_t i = 0; i < 9; i++)
		if (directive == known[i])
			return true;
	return false;
}

void validateDirectiveValue(const std::string& directive, 
                            const std::vector<std::string>& tokens)
{
	if (directive == "listen" || directive == "server_name" || 
	    directive == "root" || directive == "index" || 
	    directive == "error_page" || directive == "client_max_body_size" ||
	    directive == "client_timeout" || directive == "cgi_extension" || 
	    directive == "fastcgi_pass")
	{
		if (tokens.size() < 2)
			throw std::runtime_error(directive + " directive requires a value");
	}
}

} // namespace

std::vector<std::string> Config::split(const std::string& str, char delim)
{
	std::vector<std::string> tokens;
	std::string current;
	for (size_t i = 0; i < str.size(); ++i)
	{
		if (str[i] == delim)
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

std::vector<std::string> Config::splitTokens(const std::string& statement)
{
	return ::splitTokens(statement);
}

std::string Config::removeTrailingSemicolon(std::string line)
{
	return ::removeTrailingSemicolon(line);
}

bool Config::endsWithBrace(const std::string& line)
{
	return ::endsWithBrace(line);
}

bool Config::isKnownServerDirective(const std::string& directive)
{
	return ::isKnownServerDirective(directive);
}

void Config::validateDirectiveValue(const std::string& directive, 
                                     const std::vector<std::string>& tokens)
{
	::validateDirectiveValue(directive, tokens);
}
