#include "ext_libs.hpp"


bool is_in_set(char c, const std::string& trim_chars)
{
    for (std::size_t i = 0; i < trim_chars.length(); ++i) {
        if (c == trim_chars[i])
            return true;
    }
    return false;
}

std::string trim(const std::string& str, const std::string& trim_chars)
{
    std::size_t start = 0;
    while (start < str.length() && is_in_set(str[start], trim_chars))
        ++start;

    std::size_t end = str.length();
    while (end > start && is_in_set(str[end - 1], trim_chars))
        --end;

    return (str.substr(start, end - start));
}