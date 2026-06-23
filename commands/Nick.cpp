#include "include/NickCommand.hpp"
#include "include/Server.hpp"
#include "include/Client.hpp"

NickCommand::NickCommand() {}
NickCommand::~NickCommand() {}

void NickCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    if (params.empty())
    {
        client.sendMessage(":server 431 * :No nickname given\r\n");
        return;
    }

    std::string newNick = params;

    if (server.findClientByNick(newNick) != NULL)
    {
        std::string currentNick = client.getNickname().empty() ? "*" : client.getNickname();
        client.sendMessage(":server 433 " + currentNick + " " + newNick + " :Nickname is already in use\r\n");
        return;
    }

    std::string oldNick = client.getNickname();
    if (!oldNick.empty())
    {
        std::string nickMsg = ":" + oldNick + "!" + client.getUsername() + "@localhost NICK :" + newNick + "\r\n";
        client.sendMessage(nickMsg);
    }

    client.setNickname(newNick);
    client.setNickSet(true);
    
    if (client.isRegistered())
    {
        client.sendMessage(":server 001 " + client.getNickname() + " :Welcome to the ft_irc Network, " + client.getNickname() + "!" + client.getUsername() + "@localhost\r\n");
    }

}

Command* createNickCommand() 
{ 
    return new NickCommand(); 
}