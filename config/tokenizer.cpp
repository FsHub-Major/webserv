#include "tokenizer.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

std::string Tokenizer::readFile(const std::string &filename)
{
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        throw std::runtime_error("cannot open config file");
    // read the content of the stream into one single string, and return it
}