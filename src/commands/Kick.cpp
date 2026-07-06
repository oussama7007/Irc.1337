#include "../include/KickCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"

KickCommand::KickCommand() {}
KickCommand::~KickCommand() {}

void KickCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    if (params.size() < 2)
    {
        client.sendMessage(":server 461 " + client.getNickname() + " KICK :Not enough parameters\r\n");
        return;
    }

    std::string channelName = params[0];
    std::string targetNick = params[1];
    
    std::string reason = "No reason given";
    if (params.size() > 2)
    {
        reason = params[2];
    }

    Channel *channel = server.findChannel(channelName);
    
    if (channel == NULL)
    {
        client.sendMessage(":server 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }

    if (!channel->isMember(&client))
    {
        client.sendMessage(":server 442 " + client.getNickname() + " " + channelName + " :You're not on that channel\r\n");
        return;
    }

    if (!channel->isOperator(&client))
    {
        client.sendMessage(":server 482 " + client.getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    Client *targetClient = server.findClientByNick(targetNick);
    if (targetClient == NULL || !channel->isMember(targetClient))
    {
        client.sendMessage(":server 441 " + client.getNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n");
        return;
    }


    std::string msgToSend = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";

    channel->broadcastMessage(msgToSend, &client);
    client.sendMessage(msgToSend);        
    
    channel->removeMember(targetClient);
}

Command* createKickCommand() 
{ 
    return new KickCommand(); 
}