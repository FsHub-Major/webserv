#include "Config.hpp"

namespace
{

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

int parsePortToken(const std::string& token)
{
	std::string port_part = token;
	const size_t colon = token.find(':');
	if (colon != std::string::npos)
		port_part = token.substr(colon + 1);

	std::istringstream iss(port_part);
	int port = 0;
	if (!(iss >> port) || port <= 0 || port > 65535)
		throw std::runtime_error("Invalid port value -> " + token);

	iss >> std::ws; // reject trailing non-numeric characters like "9090a"
	if (!iss.eof())
		throw std::runtime_error("Invalid port value -> " + token);
	return port;
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

bool parseBoolToken(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), ::tolower);
	return (value == "on" || value == "true" || value == "1");
}

} // namespace

int Config::parsePortToken(const std::string& token)
{
	return ::parsePortToken(token);
}

size_t Config::parseSizeToken(const std::string& token)
{
	return ::parseSizeToken(token);
}

bool Config::parseBoolToken(const std::string& value)
{
	return ::parseBoolToken(value);
}

std::string Config::stripInlineComment(const std::string& line)
{
	return ::stripInlineComment(line);
}
