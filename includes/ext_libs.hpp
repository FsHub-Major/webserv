#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>      // for open() and O_* flags
#include <sys/stat.h>
#include <sys/time.h>   // gettimeofday

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <time.h>       // time functions
#include <vector>

std::vector<std::string> split(const std::string& s, std::string delimiters);
std::string trim(const std::string& str, const std::string& trim_chars);
int stringtoi(std::string string);
