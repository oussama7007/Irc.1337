#include "include/UserCommand.hpp"
#include "include/Server.hpp"
#include "include/Client.hpp"

UserCommand::UserCommand() {}
UserCommand::~UserCommand() {}

void UserCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    (void)server;
    if (client.isRegistered() || client.getUsername() != "")
    {
        client.sendMessage(":server 462 " + client.getNickname() + " :You may not reregister\r\n");
        return;
    }

    if (params.size() < 4)
    {
        client.sendMessage(":server 461 " + client.getNickname() + " USER :Not enough parameters\r\n");
        return;
    }

    client.setUsername(params[0]);
    client.setUserSet(true);

    if (client.isRegistered())
    {
        client.sendMessage(":server 001 " + client.getNickname() + " :Welcome to the ft_irc Network, " + client.getNickname() + "!" + client.getUsername() + "@localhost\r\n");
        
    }
}

Command* createUserCommand() 
{ 
    return new UserCommand(); 
}