#include "../include/ModeCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"
#include <cstdlib> 
#include <cstdio>
#include <iostream>
#include <sstream>
#include <cctype> // ضرورية باش نخدمو بـ isalpha

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

    // إرسال المودات الحالية ملي اليوزر كيكتب غير MODE #channel
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
            // كنطبقو المود غير يلا كانت الحالة غتتبدل بصح
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
            // كنطبقو المود غير يلا كانت الحالة غتتبدل بصح
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
                    // كنزيدو الباسورد غير يلا كانت القناة مافيهاش باسورد
                    if (channel->getPassword().empty())
                    {
                        channel->setPassword(key);
                        
                        if (lastSign != '+') { lastSign = '+'; appliedModes += '+'; }
                        appliedModes += 'k';
                        appliedParams += " " + key;
                    }
                }
            }
            else // في حالة الحذف (-k)
            {
                if (paramIndex < params.size()) 
                {
                    std::string key = params[paramIndex++];
                    // كنحيدو الباسورد غير يلا عطانا اليوزر الباسورد القديم الصحيح
                    if (channel->getPassword() == key)
                    {
                        channel->setPassword("");
                        
                        if (lastSign != '-') { lastSign = '-'; appliedModes += '-'; }
                        appliedModes += 'k';
                        // Libera كيعوض السمية ديال الباسورد بـ * للحماية
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
                    int limit = std::atoi(params[paramIndex++].c_str());
                    // كنزيدو الليميت غير يلا كان كبر من 0 ومبدل على الليميت القديم
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
                // كنحيدو الليميت غير يلا كان ديجا كاين
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
                    // كنتأكدو واش اليوزر ديجا Operator ولا لا باش ما نعاودوش العملية
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
        else
        {
            // Libera كيتجاهل الرموز بحال = بصمت، وكيجاوب غير على الحروف الغالطة
            if (std::isalpha(c))
            {
                client.sendMessage(":server 472 " + client.getNickname() + " " + std::string(1, c) + " :is unknown mode char to me\r\n");
            }
        }
    }

    // إرسال Broadcast للجميع غير يلا تبدلات شي حاجة بصح
    if (!appliedModes.empty())
    {
        std::string modeMsg = ":" + client.getNickname() + "!~" + client.getUsername() + "@localhost MODE " + target + " " + appliedModes + appliedParams + "\r\n";
        channel->broadcastMessage(modeMsg);
    }
}
//si-hamou
Command* createModeCommand() { return new ModeCommand(); }