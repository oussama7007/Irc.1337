

#include "../include/KickCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"
#include <sstream> 


KickCommand::KickCommand() {}

KickCommand::~KickCommand() {}


static std::vector<std::string> splitString(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter))
    {
        if (!token.empty()) 
            tokens.push_back(token);
    }
    return tokens;
}

//si-hamou

void KickCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    if (params.size() < 2)
    {
        client.sendMessage(":server 461 " + client.getNickname() + " KICK :Not enough parameters\r\n");
        return;
    }


    std::vector<std::string> channels = splitString(params[0], ',');
    std::vector<std::string> users = splitString(params[1], ',');
    
    std::string reason = "No reason given";
    if (params.size() > 2)
    {
        reason = params[2];
    }

    
    if (channels.size() != 1 && channels.size() != users.size())
    {
        client.sendMessage(":server 461 " + client.getNickname() + " KICK :Not enough parameters\r\n");
        return;
    }

  
    for (size_t i = 0; i < users.size(); ++i)
    {
        std::string channelName = (channels.size() == 1) ? channels[0] : channels[i];
        std::string targetNick = users[i];

        Channel *channel = server.findChannel(channelName);
        
        if (channel == NULL)
        {
            client.sendMessage(":server 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n");
            continue; 
        }

        if (!channel->isMember(&client))
        {
            client.sendMessage(":server 442 " + client.getNickname() + " " + channelName + " :You're not on that channel\r\n");
            continue;
        }

        if (!channel->isOperator(&client))
        {
            client.sendMessage(":server 482 " + client.getNickname() + " " + channelName + " :You're not channel operator\r\n");
            continue;
        }

        Client *targetClient = server.findClientByNick(targetNick);
        if (targetClient == NULL || !channel->isMember(targetClient))
        {
            client.sendMessage(":server 441 " + client.getNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n");
            continue;
        }

        std::string msgToSend = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost KICK " + channelName + " " + targetNick + " :" + reason + "\r\n";

 
        channel->broadcastMessage(msgToSend, &client);
        client.sendMessage(msgToSend);        
        
        channel->removeMember(targetClient);
    }
}

//si-hamou
Command* createKickCommand() 
{ 
    return new KickCommand(); 
}