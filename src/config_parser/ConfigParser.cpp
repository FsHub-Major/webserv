#include "Config.hpp"
#include "ext_libs.hpp"
#include "macros.hpp"
#include "webserv.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstdlib>

ServerConfig Config::parseServerBlock(std::ifstream& file, const ServerConfig& global_config)
{
	ServerConfig server = global_config;
	server.locations.clear();
	bool has_explicit_root = false;
	bool found_closing_brace = false;
	bool has_listen = false;
	std::string raw_line;

	while (std::getline(file, raw_line))
	{
		std::string current = stripInlineComment(raw_line);
		trim(current);
		if (current.empty())
			continue;

		if (current == "}")
		{
			found_closing_brace = true;
			break;
		}

		if (isLocationHeader(current))
		{
			std::string header = collectHeader(file, current);
			handleLocationHeader(file, server, header);
			continue;
		}

		if (current[current.size() - 1] != ';')
			throw std::runtime_error(
				"Directive inside server block must end with ';': " + current);

		current = removeTrailingSemicolon(current);
		trim(current);
		std::vector<std::string> tokens = splitTokens(current);
		if (tokens.empty())
			continue;

		const std::string directive = tokens[0];
		processServerDirective(server, directive, tokens, has_listen, has_explicit_root);
	}

	validateServerBlockCompletion(has_listen, has_explicit_root, 
	                               found_closing_brace, server.root);
	return server;
}

LocationConfig Config::parseLocationBlock(std::ifstream& file,
                                          const std::string& location_path)
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
			throw std::runtime_error(
				"Directive inside location block must end with ';': " + current);

		current = removeTrailingSemicolon(current);
		trim(current);
		std::vector<std::string> tokens = splitTokens(current);
		if (tokens.empty())
			continue;

		parseLocationDirective(location, tokens);
	}

	if (location.allowed_methods.empty())
		location.allowed_methods.push_back("GET");

	return location;
}

// Helper: Check if a line starts a location header
bool Config::isLocationHeader(const std::string& current)
{
	return current.compare(0, 8, "location") == 0;
}

// Helper: Collect header lines until the opening brace is found
std::string Config::collectHeader(std::ifstream& file, const std::string& header)
{
	std::string result = header;
	if (result.find('{') == std::string::npos)
	{
		std::string brace_line;
		while (std::getline(file, brace_line))
		{
			brace_line = stripInlineComment(brace_line);
			trim(brace_line);
			if (brace_line.empty())
				continue;
			result += " " + brace_line;
			if (brace_line.find('{') != std::string::npos)
				break;
		}
	}
	return result;
}

// Helper: Handle parsing of a location header and push the resulting location into server
void Config::handleLocationHeader(std::ifstream& file, ServerConfig& server, const std::string& header)
{
	std::string before_brace = header.substr(0, header.find('{'));
	trim(before_brace);
	std::vector<std::string> tokens = splitTokens(before_brace);
	if (tokens.size() < 2)
		throw std::runtime_error("Location block requires a path");
	const std::string location_path = tokens[1];
	LocationConfig location = parseLocationBlock(file, location_path);
	server.locations.push_back(location);
}

// Helper: Process a server directive (listen, root, server_name, etc.)
void Config::processServerDirective(ServerConfig& server, const std::string& directive,
									const std::vector<std::string>& tokens, bool& has_listen,
									bool& has_explicit_root)
{
	if (!isKnownServerDirective(directive))
		throw std::runtime_error("Unknown server directive: '" + directive + "'");
	validateDirectiveValue(directive, tokens);
	if (directive == "listen")
	{
		if (has_listen)
			throw std::runtime_error("Duplicate listen directive in server block");
		has_listen = true;
		server.port = parsePortToken(tokens[1]);
	}
	else if (directive == "server_name" && tokens.size() >= 2)
		server.server_name = tokens[1];
	else if (directive == "root" && tokens.size() >= 2)
	{
		server.root = tokens[1];
		has_explicit_root = true;
	}
	else if (directive == "index" && tokens.size() >= 2)
		server.index_files.assign(tokens.begin() + 1, tokens.end());
	else if (directive == "error_page" && tokens.size() >= 3)
		server.error_pages[std::atoi(tokens[1].c_str())] = tokens[2];
	else if (directive == "client_max_body_size" && tokens.size() >= 2)
		server.client_max_body_size = parseSizeToken(tokens[1]);
	else if (directive == "client_timeout" && tokens.size() >= 2)
		server.client_timeout = std::atoi(tokens[1].c_str());
}

void Config::parseLocationDirective(LocationConfig& location,
                                    const std::vector<std::string>& tokens)
{
	if (tokens.empty())
		return;

	const std::string directive = tokens[0];

	if ((directive == "path" || directive == "root") && tokens.size() >= 2)
		location.path = tokens[1];
	else if (directive == "methods" && tokens.size() >= 2)
		location.allowed_methods.assign(tokens.begin() + 1, tokens.end());
	else if (directive == "autoindex" && tokens.size() >= 2)
		location.autoindex = parseBoolToken(tokens[1]);
	else if ((directive == "upload_store" || directive == "upload_dir") && 
	         tokens.size() >= 2)
		location.upload_dir = tokens[1];
	else if ((directive == "cgi_extension" || directive == "cgi_extensions") && 
	         tokens.size() >= 2)
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
		std::cerr << "Warning: Unknown location directive '" 
		          << directive << "'" << std::endl;
}

void Config::validateServerBlockCompletion(bool has_listen, bool has_explicit_root,
                                           bool found_closing_brace, const std::string& root)
{
	if (!has_listen)
		throw std::runtime_error("Server block missing listen directive");
	if (!has_explicit_root && root.empty())
		throw std::runtime_error("Server block missing root directive");
	if (!found_closing_brace)
		throw std::runtime_error("Server block not properly closed with '}'");
}
