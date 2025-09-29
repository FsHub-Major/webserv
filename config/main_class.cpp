#include <cstdlib>
#include "main_class.hpp"
#include "server_config.hpp"
#include "tokenizer.hpp"
#include <stdexcept>

// Helper function to parse listen value (host:port or port)
const std::vector<ServerConfig>& ParsingServer::getServers() const {
    return servers;
}

std::string ParsingServer::peek() const {
    return pos < tokens.size() ? tokens[pos] : "";
}
std::string ParsingServer::get() {
    return pos < tokens.size() ? tokens[pos++] : "";
}
ParsingServer::ParsingServer(const std::string &filename)
{
    Tokenizer tokenizer;
    std::string fileContent = tokenizer.readFile(filename);
    tokens = tokenizer.tokenize(fileContent);
    pos = 0;
}

void ParsingServer::parse() {
    while (pos < tokens.size()) {
        // Skip comments
        if (peek().size() > 0 && peek()[0] == '#') {
            while (pos < tokens.size() && get() != ";") {}
            continue;
        }
        if (peek() == "server") {
            get();
            if (get() != "{")
                throw std::runtime_error("Expected '{' after 'server'");
            ServerConfig server = parseServer();
            servers.push_back(server);
        } else {
            get();
        }
    }
}
