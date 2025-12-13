#include "Config.hpp"

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
			for (std::map<int, std::string>::const_iterator ep = 
			     server.error_pages.begin();
			     ep != server.error_pages.end(); ++ep)
			{
				os << "    " << ep->first << " -> " << ep->second << std::endl;
			}
		}

		os << "  Locations: " << server.locations.size() << std::endl;
		for (size_t locIdx = 0; locIdx < server.locations.size(); ++locIdx)
		{
			const LocationConfig& loc = server.locations[locIdx];
			os << "    Location #" << (locIdx + 1) << ": " 
			   << loc.location << std::endl;
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
			os << "      Autoindex: " << (loc.autoindex ? "on" : "off") 
			   << std::endl;
			os << "      Upload Dir: " 
			   << (loc.upload_dir.empty() ? "(none)" : loc.upload_dir) 
			   << std::endl;
			os << "      CGI Path: " 
			   << (loc.cgi_path.empty() ? "(none)" : loc.cgi_path) 
			   << std::endl;
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
			os << "      FastCGI Pass: " 
			   << (loc.fastcgi_pass.empty() ? "(none)" : loc.fastcgi_pass) 
			   << std::endl;

			if (loc.has_return)
				os << "      Redirect: " << loc.return_code << " -> " 
				   << loc.return_target << std::endl;
			else
				os << "      Redirect: (none)" << std::endl;
		}

		if (i + 1 != servers.size())
			os << std::endl;
	}
	os << std::endl;
}
