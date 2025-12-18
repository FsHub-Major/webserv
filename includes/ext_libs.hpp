#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>      // for open() and O_* flags
#include <sys/stat.h>
#include <sys/time.h>   // gettimeofday

#include <climits>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <time.h>       // time functions
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <set>
#include <sstream>
#include <arpa/inet.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <csignal>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <set>
#include <sstream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <fcntl.h>

std::vector<std::string> split(const std::string& s, std::string delimiters);
std::string trim(const std::string& str, const std::string& trim_chars);
int stringtoi(std::string string);