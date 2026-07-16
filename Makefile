NAME        = ircserv
BOT_NAME    = ircbot

CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98 -g

INCLUDES    = -I . -I include

SRC_DIR     = src/

CMD_DIR     = $(SRC_DIR)commands/
CHNL_DIR    = $(SRC_DIR)channel/
CLNT_DIR    = $(SRC_DIR)client/
PRSR_DIR    = $(SRC_DIR)parser/
SRVR_DIR    = $(SRC_DIR)server/
BOT_DIR     = $(SRC_DIR)bot/

SRCS        = $(SRC_DIR)main.cpp \
              $(CHNL_DIR)Channel.cpp \
              $(CLNT_DIR)Client.cpp \
              $(PRSR_DIR)Parser.cpp \
              $(SRVR_DIR)Server.cpp \
              $(CMD_DIR)InviteCommand.cpp \
              $(CMD_DIR)Kick.cpp \
              $(CMD_DIR)Join.cpp \
              $(CMD_DIR)TopicCommand.cpp \
              $(CMD_DIR)ModeCommand.cpp \
              $(CMD_DIR)Nick.cpp \
              $(CMD_DIR)PartCommand.cpp \
              $(CMD_DIR)Pass.cpp \
              $(CMD_DIR)PrivmsgCommand.cpp \
              $(CMD_DIR)User.cpp

OBJS        = $(SRCS:.cpp=.o)

BOT_SRCS    = $(BOT_DIR)main.cpp \
              $(BOT_DIR)Bot.cpp

BOT_OBJS    = $(BOT_SRCS:.cpp=.o)

all: $(NAME)

bonus: $(NAME) $(BOT_NAME)

$(NAME): $(OBJS)
	@echo "Linking $(NAME)..."
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "Done! You can now run ./$(NAME) <port> <password>"

$(BOT_NAME): $(BOT_OBJS)
	@echo "Linking $(BOT_NAME)..."
	@$(CXX) $(CXXFLAGS) $(BOT_OBJS) -o $(BOT_NAME)
	@echo "Done! You can now run ./$(BOT_NAME) <host> <port> <password> <channel>"

%.o: %.cpp
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@echo "Cleaning objects..."
	@rm -f $(OBJS) $(BOT_OBJS)

fclean: clean
	@echo "Cleaning executable..."
	@rm -f $(NAME) $(BOT_NAME)

re: fclean all

.PHONY: all bonus clean fclean re
