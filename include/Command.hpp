// include/Command.hpp  [shared — already agreed]
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

// One factory per command, defined at the bottom of its own .cpp file.
// Add a line here every time you build a new command.
Command* createPassCommand();
Command* createNickCommand();
Command* createUserCommand();
Command* createJoinCommand();