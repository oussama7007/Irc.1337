#include "../include/TopicCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"



TopicCommand::TopicCommand() {}
TopicCommand::~TopicCommand() {}

void TopicCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    // need at least the channel name
    if (params.size() < 1)
    {
        client.sendMessage(":server 461 " + client.getNickname() + " TOPIC :Not enough parameters\r\n");
        return;
    }

    std::string channelName = params[0];

    // find the channel
    Channel *channel = server.findChannel(channelName);
    if (channel == NULL)
    {
        client.sendMessage(":server 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }

    // caller must be a member
    if (!channel->isMember(&client))
    {
        client.sendMessage(":server 442 " + client.getNickname() + " " + channelName + " :You're not on that channel\r\n");
        return;
    }

    // VIEW mode - no new topic provided
    if (params.size() == 1)
    {
        std::string topic = channel->getTopic();
        if (topic.empty())
            client.sendMessage(":server 331 " + client.getNickname() + " " + channelName + " :No topic is set\r\n");
        else
            client.sendMessage(":server 332 " + client.getNickname() + " " + channelName + " :" + topic + "\r\n");
        return;
    }

    // CHANGE mode - new topic provided
    // if +t is ON, only operators can change the topic
    if (channel->isTopicRestricted() && !channel->isOperator(&client))
    {
        client.sendMessage(":server 482 " + client.getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    // set the new topic
    std::string newTopic = params[1];
    channel->setTopic(newTopic);

    // broadcast to everyone in the channel including the sender
    std::string msg = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost TOPIC " + channelName + " :" + newTopic + "\r\n";
    channel->broadcastMessage(msg);
    client.sendMessage(msg);
}

Command* createTopicCommand()
{
    return new TopicCommand();
}