#include "../include/ModeCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"
#include <cstdlib> 
#include <cstdio>
#include <iostream>
#include <sstream>
#include <cctype> 

ModeCommand::ModeCommand() {}
ModeCommand::~ModeCommand() {}

void ModeCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    if (params.empty())
    {
        client.sendMessage(":server 461 " + client.getNickname() + " MODE :Not enough parameters\r\n");
        return;
    }

    std::string target = params[0];

    if (target[0] != '#' && target[0] != '&') return;

    Channel *channel = server.findChannel(target);
    if (channel == NULL)
    {
        client.sendMessage(":server 403 " + client.getNickname() + " " + target + " :No such channel\r\n");
        return;
    }

    if (params.size() == 1)
    {
        
        std::string activeModes = "+";
        std::string modeArguments = "";

        if (channel->isInviteOnly()) activeModes += "i";
        if (channel->isTopicRestricted()) activeModes += "t";
        
        if (!channel->getPassword().empty()) 
        {
            activeModes += "k";
            modeArguments += " " + channel->getPassword();
        }
        
        if (channel->getUserLimit() > 0) 
        {
            activeModes += "l";
            std::stringstream ss;
            ss << channel->getUserLimit();
            modeArguments += " " + ss.str();
        }
        
        if (activeModes == "+") activeModes = "";

        client.sendMessage(":server 324 " + client.getNickname() + " " + target + " " + activeModes + modeArguments + "\r\n"); 
        return;
    }

    if (!channel->isOperator(&client))
    {
        client.sendMessage(":server 482 " + client.getNickname() + " " + target + " :You're not channel operator\r\n");
        return;
    }

    std::string modeString = params[1];


    for (size_t i = 0; i < modeString.length(); ++i)
    {
        char c = modeString[i];
        if (c != '+' && c != '-' && c != 'i' && c != 't' && c != 'k' && c != 'l' && c != 'o')
        {
            if (std::isalpha(c)) 
            {
                client.sendMessage(":server 472 " + client.getNickname() + " " + std::string(1, c) + " :is not a recognised channel mode.\r\n");
                return; 
            }
        }
    }

    bool isPlus = true;
    size_t paramIndex = 2; 

    std::string appliedModes = "";
    std::string appliedParams = "";
    char lastSign = '\0'; 

    for (size_t i = 0; i < modeString.length(); ++i)
    {
        char c = modeString[i];

        if (c == '+') 
        { 
            isPlus = true; 
        }
        else if (c == '-') 
        { 
            isPlus = false; 
        }
        else if (c == 'i') 
        { 
            if (channel->isInviteOnly() != isPlus)
            {
                channel->setInviteOnly(isPlus); 
                
                if (lastSign != (isPlus ? '+' : '-')) {
                    lastSign = (isPlus ? '+' : '-');
                    appliedModes += lastSign;
                }
                appliedModes += 'i'; 
            }
        }
        else if (c == 't') 
        { 
            if (channel->isTopicRestricted() != isPlus)
            {
                channel->setTopicRestricted(isPlus); 
                
                if (lastSign != (isPlus ? '+' : '-')) {
                    lastSign = (isPlus ? '+' : '-');
                    appliedModes += lastSign;
                }
                appliedModes += 't'; 
            }
        }
        else if (c == 'k') 
        {
            if (isPlus) 
            {
                if (paramIndex < params.size()) 
                {
                    std::string key = params[paramIndex++];
                    
                    if (key.find(' ') != std::string::npos || key.find('\t') != std::string::npos || key.find(',') != std::string::npos)
                    {
                        client.sendMessage(":server 696 " + client.getNickname() + " " + target + " k " + key + " :Invalid channel key (cannot contain spaces)\r\n");
                        continue;
                    }
                    
                    if (channel->getPassword().empty())
                    {
                        channel->setPassword(key);
                        
                        if (lastSign != '+') { lastSign = '+'; appliedModes += '+'; }
                        appliedModes += 'k';
                        appliedParams += " " + key;
                    }
                    else
                    {
                        client.sendMessage(":server 467 " + client.getNickname() + " " + target + " :Channel key already set\r\n");
                    }
                }
            }
            else 
            {
                if (paramIndex < params.size()) 
                {
                    std::string key = params[paramIndex++];
                    
                    if (channel->getPassword() == key)
                    {
                        channel->setPassword("");
                        
                        if (lastSign != '-') { lastSign = '-'; appliedModes += '-'; }
                        appliedModes += 'k';
                        appliedParams += " *"; 
                    }
                }
            }
        }
        else if (c == 'l') 
        {
            if (isPlus) 
            {
                if (paramIndex < params.size()) 
                {
                    std::string limitStr = params[paramIndex++];
                    bool isValidNumber = true;

                    for (size_t j = 0; j < limitStr.length(); ++j)
                    {
                        if (!std::isdigit(limitStr[j]))
                        {
                            isValidNumber = false;
                            break;
                        }
                    }
                    if (!isValidNumber || limitStr.empty())
                    {
                        client.sendMessage(":server 696 " + client.getNickname() + " " + target + " l " + limitStr + " :Invalid limit mode parameter. Syntax: <limit>.\r\n");
                        continue; 
                    }
                    int limit = std::atoi(limitStr.c_str());
                    if (limit > 0 && channel->getUserLimit() != (size_t)limit)
                    {
                        channel->setUserLimit(limit);
                        
                        if (lastSign != '+') { lastSign = '+'; appliedModes += '+'; }
                        appliedModes += 'l';
                        
                        std::stringstream ss;
                        ss << limit;
                        appliedParams += " " + ss.str();
                    }
                }
            }
            else 
            {
                if (channel->getUserLimit() > 0)
                {
                    channel->setUserLimit(0);
                    
                    if (lastSign != '-') { lastSign = '-'; appliedModes += '-'; }
                    appliedModes += 'l';
                }
            }
        }
        else if (c == 'o') 
        {
            if (paramIndex < params.size())
            {
                std::string targetNick = params[paramIndex++];
                Client *targetClient = server.findClientByNick(targetNick);
                
                if (targetClient && channel->isMember(targetClient))
                {
                    bool isOp = channel->isOperator(targetClient);
                    
                    if ((isPlus && !isOp) || (!isPlus && isOp))
                    {
                        if (isPlus) channel->addOperator(targetClient);
                        else channel->removeOperator(targetClient);
                        
                        char neededSign = isPlus ? '+' : '-';
                        if (lastSign != neededSign) {
                            lastSign = neededSign;
                            appliedModes += lastSign;
                        }
                        appliedModes += 'o';
                        appliedParams += " " + targetNick;
                    }
                }
                else
                {
                    client.sendMessage(":server 441 " + client.getNickname() + " " + targetNick + " " + target + " :They aren't on that channel\r\n");
                }
            }
        }
        
    }

    if (!appliedModes.empty())
    {
        std::string modeMsg = ":" + client.getNickname() + "!~" + client.getUsername() + "@localhost MODE " + target + " " + appliedModes + appliedParams + "\r\n";
        channel->broadcastMessage(modeMsg);
    }
}
Command* createModeCommand() { return new ModeCommand(); }