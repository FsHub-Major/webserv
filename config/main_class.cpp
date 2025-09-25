#include <cstdlib>

// Helper function to parse listen value (host:port or port)
void parseListenValue(const std::string& value, std::string& host, int& port) {
    size_t colon = value.find(':');
    if (colon != std::string::npos) {
        host = value.substr(0, colon);
        port = atoi(value.substr(colon + 1).c_str());
    } else {
        host = "0.0.0.0";
        port = atoi(value.c_str());
    }
}
#include "main_class.hpp"
#include "server_config.hpp"
#include "tokenizer.hpp"
#include <stdexcept>

void parseListenValue(const std::string& value, std::string& host, int& port) {
    size_t colon = value.find(':');
    if (colon != std::string::npos) {
        host = value.substr(0, colon);
        port = atoi(value.substr(colon + 1).c_str());
    } else {
        host = "0.0.0.0";
        port = atoi(value.c_str());
    }
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

ServerConfig ParsingServer::parseServer() {
    ServerConfig server;
    while (pos < tokens.size()) {
        std::string token = get();
        if (token == "}") break;
        if (token.size() > 0 && token[0] == '#') {
            while (pos < tokens.size() && get() != ";") {}
            continue;
        }
        if (token == "listen") {
            std::string value = get();
            std::string host;
            int port;
            parseListenValue(value, host, port);
            server.addListen(host, port);
            if (peek() == ";") get();
        } else if (token == "server_name") {
            std::string value = get();
            server.setServerName(value);
            if (peek() == ";") get();
        } else if (token == "client_max_body_size") {
            std::string value = get();
            server.setMaxBodySize(atoi(value.c_str()));
            if (peek() == ";") get();
        } else if (token == "location") {
            std::string path = get();
            if (get() != "{")
                throw std::runtime_error("Expected '{' after 'location'");
            Location loc = parseLocation(path);
            server.addLocation(loc);
        } else {
            std::vector<std::string> values;
            while (peek() != ";" && peek() != "}") {
                values.push_back(get());
            }
            if (peek() == ";") get();
            server.addDirective(token, values);
        }
    }
    return server;
}