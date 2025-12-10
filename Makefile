# Program name
NAME =  webserv

# C++ compiler with all flags
CXX = c++
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror -g
## Allow extra flags from wrapper targets
CXXFLAGS += $(EXTRA_FLAGS)

# Directories
OBJ_DIR = objects
SRC_DIR = src

#include
INC= -I includes

# Source and Object Files - explicit list to avoid duplicate sources
SRC = \
	$(SRC_DIR)/webserv.cpp \
	$(SRC_DIR)/Server.cpp \
	$(SRC_DIR)/ClientManager.cpp \
	$(SRC_DIR)/Config.cpp \
	$(SRC_DIR)/FastCgiClient.cpp \
	$(SRC_DIR)/http/HttpRequest.cpp \
	$(SRC_DIR)/http/HttpResponse.cpp \
	$(SRC_DIR)/utils/split.cpp \
	$(SRC_DIR)/utils/stringtoi.cpp \
	$(SRC_DIR)/utils/trim.cpp

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

# ------------------------------------------------------
# Simulation targets (toggle OS macros at compile time)
# ------------------------------------------------------
.PHONY: sim-macos sim-linux

# Simulate macOS build on any host: enable __APPLE__, disable __linux__
sim-macos: clean
	@$(MAKE) EXTRA_FLAGS="-D__APPLE__ -U__linux__" all

# Simulate Linux build on any host: enable __linux__, disable __APPLE__
# NOTE: On macOS this will likely fail without Linux headers/toolchain.
sim-linux: clean
	@$(MAKE) EXTRA_FLAGS="-D__linux__ -U__APPLE__" all

