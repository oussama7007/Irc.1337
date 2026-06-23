



#include "include/JoinCommand.hpp"
#include "include/Server.hpp"
#include "include/Client.hpp"
#include "include/Channel.hpp"


JoinCommand::JoinCommand() {}
JoinCommand::~JoinCommand() {}


void JoinCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    if (params.empty())
    {
        client.sendMessage(":server 461 " + client.getNickname() + " JOIN :Not enough parameters\r\n");
        return;
    }

    std::string channelName = params;
    Channel *channel = server.findChannel(channelName);
    bool isNewChannel = false;

    if (channel == NULL)
    {
        channel = server.createChannel(channelName);
        isNewChannel = true;
    }

    channel->addMember(&client);

    if (isNewChannel)
    {
        channel->addOperator(&client);
    }

    std::string userPrefix = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost";
    std::string joinMsg = userPrefix + " JOIN :" + channelName + "\r\n";

   
    channel->broadcastMessage(joinMsg, &client); 
    
  
    client.sendMessage(joinMsg);

    if (!channel->getTopic().empty())
    {
        client.sendMessage(":server 332 " + client.getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");
    }
}


Command* createJoinCommand() 
{ 
    return new JoinCommand(); 
}