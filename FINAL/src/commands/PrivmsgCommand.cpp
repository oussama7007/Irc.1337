#include "../include/PrivmsgCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"
#include <sstream>

PrivmsgCommand::PrivmsgCommand() {}

PrivmsgCommand::~PrivmsgCommand() {}


static std::vector<std::string> splitString(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter))
    {
        if (!token.empty()) 
        {
            tokens.push_back(token);
        }
    }
    return tokens;
}


static bool isChannel(const std::string &target)
{
    if (target.empty()) 
    {
        return false;
    }

    char firstChar = target[0];
    

    if (firstChar == '#' || firstChar == '&') 
    {
        return true;
    }
    
    return false;
}


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
 

    std::string targetString = params[0];
    std::string messageText = params[1];
    
    std::vector<std::string> targetList = splitString(targetString, ',');

  
    std::string senderPrefix = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost";


    for (size_t i = 0; i < targetList.size(); ++i)
    {
        std::string currentTarget = targetList[i];
        

        std::string fullMessage = senderPrefix + " PRIVMSG " + currentTarget + " :" + messageText + "\r\n";

        if (isChannel(currentTarget))
        {
            Channel *channel = server.findChannel(currentTarget);
            
          
            if (channel == NULL)
            {
                client.sendMessage(":server 401 " + client.getNickname() + " " + currentTarget + " :No such nick/channel\r\n");
                continue; 
            }

      
            if (!channel->isMember(&client))
            {
                client.sendMessage(":server 404 " + client.getNickname() + " " + currentTarget + " :Cannot send to channel\r\n");
                continue; 
            }

         
            channel->broadcastMessage(fullMessage, &client);
        }
        

        else 
        {
            Client *targetClient = server.findClientByNick(currentTarget);
            
   
            if (targetClient == NULL)
            {
                client.sendMessage(":server 401 " + client.getNickname() + " " + currentTarget + " :No such nick/channel\r\n");
                continue; 
            }
            
       
            targetClient->sendMessage(fullMessage);
        }
    }
}

Command* createPrivmsgCommand() 
{ 
    return new PrivmsgCommand(); 
}