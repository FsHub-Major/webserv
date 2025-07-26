#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <climits>
#include <sstream>
#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <cstdio> 
#include <time.h> // time functions 
#include <sys/time.h> //gettimeofday

std::vector<std::string> split(const std::string& s, std::string delimiters);
int stringtoi(std::string string);