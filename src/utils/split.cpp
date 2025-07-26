#include "ext_libs.hpp"

std::vector<std::string> split(const std::string& s, std::string delimiters)
{
    std::vector<std::string> tokens;
    size_t last = 0;
    size_t next = 0;
    
    while ((next = s.find_first_of(delimiters, last)) != std::string::npos)
    {
        if (next - last > 0)
            tokens.push_back(s.substr(last, next - last));
        last = next + 1;
    }
    
    // Add the last token if there is one
    if (last < s.length())
        tokens.push_back(s.substr(last));
    
    return tokens;
}
