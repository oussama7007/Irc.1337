NAME        = ircserv

CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98 -g

INCLUDES    = -I . -I include

SRC_DIR     = src/

CMD_DIR     = $(SRC_DIR)commands/
CHNL_DIR    = $(SRC_DIR)channel/
CLNT_DIR    = $(SRC_DIR)client/
PRSR_DIR    = $(SRC_DIR)parser/
SRVR_DIR    = $(SRC_DIR)server/

SRCS        = $(SRC_DIR)main.cpp \
              $(CHNL_DIR)Channel.cpp \
              $(CLNT_DIR)Client.cpp \
              $(PRSR_DIR)Parser.cpp \
              $(SRVR_DIR)Server.cpp \
              $(CMD_DIR)InviteCommand.cpp \
              $(CMD_DIR)Kick.cpp \
              $(CMD_DIR)Join.cpp \
              $(CMD_DIR)ModeCommand.cpp \
              $(CMD_DIR)Nick.cpp \
              $(CMD_DIR)PartCommand.cpp \
              $(CMD_DIR)Pass.cpp \
              $(CMD_DIR)PrivmsgCommand.cpp \
              $(CMD_DIR)User.cpp

OBJS        = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	@echo "Linking $(NAME)..."
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "Done! You can now run ./$(NAME) <port> <password>"

%.o: %.cpp
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@echo "Cleaning objects..."
	@rm -f $(OBJS)

fclean: clean
	@echo "Cleaning executable..."
	@rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re