#include "include/InviteCommand.hpp"
#include "include/Server.hpp"
#include "include/Client.hpp"
#include "include/Channel.hpp"

InviteCommand::InviteCommand() {}
InviteCommand::~InviteCommand() {}

void InviteCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    if (params.size() < 2)
    {
        client.sendMessage(":server 461 " + client.getNickname() + " INVITE :Not enough parameters\r\n");
        return;
    }

    std::string targetNick = params;
    std::string channelName = params[1];

    Client *targetClient = server.findClientByNick(targetNick);
    if (targetClient == NULL)
    {
        client.sendMessage(":server 401 " + client.getNickname() + " " + targetNick + " :No such nick/channel\r\n");
        return;
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

    if (channel->isMember(targetClient))
    {
        client.sendMessage(":server 443 " + client.getNickname() + " " + targetNick + " " + channelName + " :is already on channel\r\n");
        return;
    }

    if (!channel->isOperator(&client))
    {
        client.sendMessage(":server 482 " + client.getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }


    channel->addInvited(targetClient);

    client.sendMessage(":server 341 " + client.getNickname() + " " + targetNick + " " + channelName + "\r\n");

    std::string inviteMsg = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost INVITE " + targetNick + " :" + channelName + "\r\n";
    targetClient->sendMessage(inviteMsg);
}

Command* createInviteCommand() 
{ 
    return new InviteCommand(); 
}