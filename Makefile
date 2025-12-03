CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++17
NAME := ircserv

SRC_DIR := src
INC_DIR := include

SRC := $(SRC_DIR)/main.cpp \
       $(SRC_DIR)/Server.cpp \
	   $(SRC_DIR)/Parser.cpp \
	   $(SRC_DIR)/Client.cpp
OBJ := $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
