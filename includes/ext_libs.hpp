#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>      // for open() and O_* flags
#include <vector>
#include <climits>
#include <sys/stat.h>
#include <sstream>
#include <map>
#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <cstdio> 
#include <time.h> // time functions 
#include <sys/time.h> //gettimeofday

std::vector<std::string> split(const std::string& s, std::string delimiters);
std::string trim(const std::string& str, const std::string& trim_chars);
int stringtoi(std::string string);