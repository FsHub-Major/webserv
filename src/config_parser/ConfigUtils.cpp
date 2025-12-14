#include "Config.hpp"

namespace ConfigUtils {

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

void trim(std::string& str)
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

} // namespace ConfigUtils
