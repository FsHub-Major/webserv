NAME = parsing
CXX = c++ 
CXXFLAGS =  -Wall -Wextra -Werror -std=c++98  #-fsanitize=address -g3
src =  main.cpp tokenizer.cpp server_config.cpp  main_class.cpp

OBJ = $(src:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

clean: 
	rm -rf $(OBJ)

fclean: clean
	rm -rf $(NAME)

re: fclean all