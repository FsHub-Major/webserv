#include "ext_libs.hpp"

int stringtoi(std::string string)
{
    int result = 0;
    int sign = 1;
    size_t i = 0;
    
    while (i < string.length() && (string[i] == ' ' || string[i] == '\t' || 
           string[i] == '\n' || string[i] == '\v' || 
           string[i] == '\f' || string[i] == '\r'))
        i++;
    
    if (i < string.length() && (string[i] == '-' || string[i] == '+'))
    {
        sign = (string[i] == '-') ? -1 : 1;
        i++;
    }
    
    while (i < string.length() && string[i] >= '0' && string[i] <= '9')
    {
        if (result > INT_MAX / 10 || 
           (result == INT_MAX / 10 && string[i] - '0' > INT_MAX % 10))
        {
            return (sign == 1) ? INT_MAX : INT_MIN;
        }
        
        result = result * 10 + (string[i] - '0');
        i++;
    }
    
    return result * sign;
}
