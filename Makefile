# Program name
NAME =  webserv

# C++ compiler with all flags
CXX = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror -g

# Directories
OBJ_DIR = objects
SRC_DIR = src

#include
INC= -I includes

# Source and Object Files - recursive search
SRC = $(shell find $(SRC_DIR) -name "*.cpp" -type f)
OBJ = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC))

# Debugging
ifeq ($(DEBUG), GDB)
    CXXFLAGS += -g
else ifeq ($(DEBUG), ALL)
    CXXFLAGS += -g -fsanitize=address
endif

.PHONY: all clean fclean re clear

all: $(NAME)

$(NAME): $(OBJ)
		@$(CXX)  $(INC) $(CXXFLAGS) $(OBJ) -o $@
		@echo "compiling"
		@sleep 0.5
		@echo "$(NAME) is ready"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
		@mkdir -p $(dir $@)
		@$(CXX) $(INC) $(CXXFLAGS) -c $< -o $@

clean:
		@rm -rf $(OBJ_DIR)
		@echo "cleaning..."

fclean: clean
		@rm -f $(NAME)
		@echo "cleaning program..."

re: fclean all

clear: all clean

bclear: all clean

