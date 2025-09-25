#include "tokenizer.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

// read the content of the stream into one single string, and return it
std::string Tokenizer::readFile(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        throw std::runtime_error("cannot open config file");
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::string> Tokenizer::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string token;
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (std::isspace(c)) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else if (c == '{' || c == '}' || c == ';') {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            tokens.push_back(std::string(1, c));
        } else {
            token += c;
        }
    }
    if (!token.empty())
        tokens.push_back(token);
    return tokens;
}