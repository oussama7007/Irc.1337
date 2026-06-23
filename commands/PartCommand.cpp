#include "include/PartCommand.hpp"
#include "include/Server.hpp"
#include "include/Client.hpp"
#include "include/Channel.hpp"

PartCommand::PartCommand() {}
PartCommand::~PartCommand() {}

void PartCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    if (params.empty())
    {
        client.sendMessage(":server 461 " + client.getNickname() + " PART :Not enough parameters\r\n");
        return;
    }

    std::string channelName = params;
    
    std::string partMessage = "Leaving"; 
    if (params.size() > 1)
    {
        partMessage = params[1];
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

    std::string msgToSend = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost PART " + channelName + " :" + partMessage + "\r\n";

    channel->broadcastMessage(msgToSend, &client);
    
    client.sendMessage(msgToSend);

    channel->removeMember(&client);

}

Command* createPartCommand() 
{ 
    return new PartCommand(); 
}