
#pragma once
#include <string>
#include <vector>

class Server;
class Client;

class Command
{
    public:
        virtual ~Command() {}
        virtual void execute(Server &server, Client &client, const std::vector<std::string> &params) = 0;
};


Command* createPassCommand();
Command* createNickCommand();
Command* createUserCommand();
Command* createJoinCommand();
Command* createTopicCommand();

Command* createPrivmsgCommand();
Command* createKickCommand();
Command* createInviteCommand();

Command* createModeCommand();
