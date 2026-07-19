#include "../include/TopicCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"



//si-hamou
TopicCommand::TopicCommand() {}
//si-hamou
TopicCommand::~TopicCommand() {}

//si-hamou

void TopicCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{

    if (params.size() < 1)
    {
        client.sendMessage(":server 461 " + client.getNickname() + " TOPIC :Not enough parameters\r\n");
        return;
    }

    std::string channelName = params[0];

 
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


    if (params.size() == 1)
    {
        std::string topic = channel->getTopic();
        if (topic.empty())
            client.sendMessage(":server 331 " + client.getNickname() + " " + channelName + " :No topic is set\r\n");
        else
            client.sendMessage(":server 332 " + client.getNickname() + " " + channelName + " :" + topic + "\r\n");
        return;
    }


    if (channel->isTopicRestricted() && !channel->isOperator(&client))
    {
        client.sendMessage(":server 482 " + client.getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

  
    std::string newTopic = params[1];
    channel->setTopic(newTopic);

   
    std::string msg = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost TOPIC " + channelName + " :" + newTopic + "\r\n";
    channel->broadcastMessage(msg, &client);
    client.sendMessage(msg);
}

//si-hamou
Command* createTopicCommand()
{
    return new TopicCommand();
}
