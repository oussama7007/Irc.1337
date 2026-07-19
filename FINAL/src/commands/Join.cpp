#include "../include/JoinCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"
#include <sstream>

JoinCommand::JoinCommand() {}

JoinCommand::~JoinCommand() {}


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


static bool isValidChannelName(const std::string &name)
{
    if (name.empty()) 
    {
        return false;
    }
    if (name.length() > 50) 
    {
        return false;
    }

    char firstChar = name[0];
    if (firstChar != '#' && firstChar != '&')
    {
        return false;
    }

    for (size_t i = 0; i < name.length(); ++i)
    {
        char c = name[i];
        if (c == ' ' || c == ',' || c == 7)
        {
            return false;
        }
    }
    return true;
}
    

void JoinCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
  
    if (params.empty())
    {
        client.sendMessage(":server 461 " + client.getNickname() + " JOIN :Not enough parameters\r\n");
        return;
    }
    if (params[0] == "0")
    {
        std::map<std::string, Channel*> allChannels = server.getChannels();
        std::map<std::string, Channel*>::iterator it;
        
        for (it = allChannels.begin(); it != allChannels.end(); ++it)
        {
            Channel *channel = it->second;
            
            if (channel->isMember(&client))
            {
                
                std::string partMsg = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost PART " + channel->getName() + " :Left all channels\r\n";
                
             
                channel->broadcastMessage(partMsg, &client);
                
                
                client.sendMessage(partMsg);
                
               
                channel->removeMember(&client);
                
              
                if (channel->getMembersCount() == 0)
                {
                    server.removeChannel(channel->getName());
                }
            }
        }
        return; 
    }
 
    std::vector<std::string> channels = splitString(params[0], ',');
    
   
    std::vector<std::string> keys;
    if (params.size() > 1)
    {
        keys = splitString(params[1], ',');
    }
    for (size_t i = 0; i < channels.size(); ++i)
    {
        std::string channelName = channels[i];
        
        std::string providedKey = "";
        
        if (i < keys.size())
        {
            providedKey = keys[i];
        }
        else
        {
            providedKey = "";
        }



        if (!isValidChannelName(channelName))
        {
            client.sendMessage(":server 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n");
            continue; 
        }   
        
        Channel *channel = server.findChannel(channelName);
        bool isNewChannel = false;

       
        if (channel == NULL)
        {
            channel = server.createChannel(channelName);
            isNewChannel = true;
            channel->setTopicRestricted(true);
        }
        else 
        {
           
            if (channel->isMember(&client))
            {
                continue; 
            }

           
            if (!channel->getPassword().empty())
            {
                if (channel->getPassword() != providedKey)
                {
                    client.sendMessage(":server 475 " + client.getNickname() + " " + channelName + " :Cannot join channel (+k) - bad key\r\n");
                    continue;
                }
            }

        
            if (channel->isInviteOnly())
            {
                if (!channel->isInvited(&client))
                {
                    client.sendMessage(":server 473 " + client.getNickname() + " " + channelName + " :Cannot join channel (+i) - you must be invited\r\n");
                    continue;
                }
            }

        
            if (channel->getUserLimit() > 0)
            {
                if (channel->getMembersCount() >= channel->getUserLimit())
                {
                    client.sendMessage(":server 471 " + client.getNickname() + " " + channelName + " :Cannot join channel (+l) - channel is full\r\n");
                    continue;
                }
            }
        }

    
        channel->addMember(&client);
        if (isNewChannel)
        {
            channel->addOperator(&client);
        }
        else 
        {
            if (channel->isInvited(&client))
            {
                channel->removeInvited(&client);
            }
        }

       
        std::string userPrefix = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost";
        std::string joinMsg = userPrefix + " JOIN :" + channelName + "\r\n";

        channel->broadcastMessage(joinMsg, &client); 
        client.sendMessage(joinMsg);


        if (isNewChannel)
        {
            std::string modeMsg = ":server MODE " + channelName + " +t\r\n";
            client.sendMessage(modeMsg);
        }

   
        if (!channel->getTopic().empty())
        {
            client.sendMessage(":server 332 " + client.getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");
        }
        // else 
        // {
        //     client.sendMessage(":server 331 " + client.getNickname() + " " + channelName + " :No topic is set\r\n");
        // }
            

        std::vector<Client*> members = channel->getMembers();
        std::string namesList = "";
        
        for (size_t j = 0; j < members.size(); ++j)
        {
            if (channel->isOperator(members[j]))
            {
                namesList += "@";
            }
            
            namesList += members[j]->getNickname();
            
 
            if (j + 1 < members.size())
            {
                namesList += " ";
            }
        }
        
        client.sendMessage(":server 353 " + client.getNickname() + " = " + channelName + " :" + namesList + "\r\n");
        client.sendMessage(":server 366 " + client.getNickname() + " " + channelName + " :End of /NAMES list\r\n");
    }
}    

Command* createJoinCommand() 
{ 
    return new JoinCommand(); 
}