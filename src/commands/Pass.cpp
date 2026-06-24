// src/commands/Pass.cpp  [PARTNER'S PART — placeholder]
#include "Command.hpp"
#include "Server.hpp"
#include "Client.hpp"

class PassCommand : public Command
{
    public:
        void execute(Server &server, Client &client, const std::vector<std::string> &params)
        {
            if (params.empty())
            {
                client.sendMessage(":server 461 * PASS :Not enough parameters\r\n");
                return;
            }
            if (params[0] == server.getPassword())
                client.setPassOk(true);
            else
                client.sendMessage(":server 464 * :Password incorrect\r\n");
        }
};

Command* createPassCommand() { return new PassCommand(); }