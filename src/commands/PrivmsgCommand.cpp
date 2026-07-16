#include "../include/PrivmsgCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"

//si-hamou
PrivmsgCommand::PrivmsgCommand() {}
//si-hamou
PrivmsgCommand::~PrivmsgCommand() {}

//si-hamou
// Fix PRIVMSG by rejecting an empty trailing message with 412 and processing
// every comma-separated target independently without duplicating recipients.
void PrivmsgCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    if (params.empty())
    {
        client.sendMessage(":server 411 " + client.getNickname() + " :No recipient given (PRIVMSG)\r\n");
        return;
    }
    if (params.size() < 2)
    {
        client.sendMessage(":server 412 " + client.getNickname() + " :No text to send\r\n");
        return;
    }
 
    std::string target = params[0];
    std::string message = params[1];

    std::string msgToSend = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost PRIVMSG " + target + " :" + message + "\r\n";

    if (target[0] == '#' || target[0] == '+' || target[0] == '!' || target[0] == '&')
    {
        Channel *channel = server.findChannel(target);
        
        if (channel == NULL)
        {
            client.sendMessage(":server 401 " + client.getNickname() + " " + target + " :No such nick/channel\r\n");
            return;
        }

        if (!channel->isMember(&client))
        {
            client.sendMessage(":server 404 " + client.getNickname() + " " + target + " :Cannot send to channel\r\n");
            return;
        }

        channel->broadcastMessage(msgToSend, &client);
    }
    else 
    {
        Client *targetClient = server.findClientByNick(target);
        if (targetClient == NULL)
        {
            client.sendMessage(":server 401 " + client.getNickname() + " " + target + " :No such nick/channel\r\n");
            return;
        }
        targetClient->sendMessage(msgToSend);
    }
}

//si-hamou
Command* createPrivmsgCommand() 
{ 
    return new PrivmsgCommand(); 
}
