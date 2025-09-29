#include "main_class.hpp"
#include "tokenizer.hpp"
#include "server_config.hpp"
#include <stdexcept>
#include <cstdlib>

void parseListenValue(const std::string& token, std::string& host, int& port) {
    size_t pos = token.find(':');

    if (pos != std::string::npos) {
        // host:port format
        host = token.substr(0, pos);
        std::string portStr = token.substr(pos + 1);
        port = std::atoi(portStr.c_str());
    } else {
        // only port given
        host = "0.0.0.0"; // default = any interface
        port = std::atoi(token.c_str());
    }

    if (port <= 0 || port > 65535) {
        throw std::runtime_error("Invalid port number in listen directive: " + token);
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

Location ParsingServer::parseLocation(const std::string& path) {
    Location loc;
    loc.setPath(path);
    while (pos < tokens.size()) {
        std::string token = get();
        if (token == "}") {
            break;
        }
        if (token.size() > 0 && token[0] == '#') {
            while (pos < tokens.size() && get() != ";") {}
            continue;
        }
        if (token == "methods") {
            std::vector<std::string> methods;
            while (peek() != ";" && peek() != "}") {
                methods.push_back(get());
            }
            loc.setMethods(methods);
            if (peek() == ";") get();
        } else if (token == "autoindex") {
            std::string value = get();
            loc.setAutoindex(value == "on");
            if (peek() == ";") get();
        } else {
            std::vector<std::string> values;
            while (peek() != ";" && peek() != "}") {
                values.push_back(get());
            }
            if (peek() == ";") get();
            loc.addDirective(token, values);
        }
    }
    return loc;
}