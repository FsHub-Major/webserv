#include "tokenizer.hpp"
#include "server_config.hpp"
#include <iostream>
#include <string>
#include <vector>

int main() {
    std::string configContent = Tokenizer::readFile("real_config_example.config");
    std::vector<std::string> tokens = Tokenizer::tokenize(configContent);

    for (size_t i = 0; i < tokens.size(); ++i) {
        std::cout << "Token: " << tokens[i] << std::endl;
    }
    return 0;
}