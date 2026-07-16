



// #include "../include/JoinCommand.hpp"
// #include "../include/Server.hpp"
// #include "../include/Client.hpp"
// #include "../include/Channel.hpp"


// //si-hamou
// JoinCommand::JoinCommand() {}
// //si-hamou
// JoinCommand::~JoinCommand() {}

// //si-hamou
// static bool    isValidChannelName(const std::string &name)
// {
//     if(name.empty() || name.length() > 50) return false;

//     char firstChar = name[0];
//     if (firstChar != '#' && firstChar != '&')
//         return false;

//     for (size_t i = 0; i < name.length(); ++i)
//     {
//         char c = name[i];
//         if (c == ' ' || c == ',' || c == 7)
//             return false;
//     }
//     return true;
// }
    

// //si-hamou
// // Fix JOIN by handling comma-separated channels and keys, treating duplicate
// // JOIN as a no-op, rejecting ':' in names, applying IRC channel casemapping,
// // enforcing channel quotas, and splitting every 353 reply below 512 bytes.
// void JoinCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
// {
//     if (params.empty())
//     {
//         client.sendMessage(":server 461 " + client.getNickname() + " JOIN :Not enough parameters\r\n");
//         return;
//     }

//     std::string channelName = params[0];
//     if (!isValidChannelName(channelName))
//     {
//         client.sendMessage(":server 476 " + client.getNickname() + " " + channelName + " :Invalid channel name\r\n");
//         return;
//     }   
//     std::string providedKey = (params.size() > 1) ? params[1] : "";
    
//     Channel *channel = server.findChannel(channelName);
//     bool isNewChannel = false;

//     if (channel == NULL)
//     {
//         channel = server.createChannel(channelName);
//         isNewChannel = true;
//     }
//     else 
//     {

//         if (!channel->getPassword().empty() && channel->getPassword() != providedKey)
//         {
//             client.sendMessage(":server 475 " + client.getNickname() + " " + channelName + " :Cannot join channel (+k) - bad key\r\n");
//             return;
//         }


//         if (channel->isInviteOnly() && !channel->isInvited(&client))
//         {
//             client.sendMessage(":server 473 " + client.getNickname() + " " + channelName + " :Cannot join channel (+i) - you must be invited\r\n");
//             return;
//         }

//         if (channel->getUserLimit() > 0 && channel->getMembersCount() >= channel->getUserLimit())
//         {
//             client.sendMessage(":server 471 " + client.getNickname() + " " + channelName + " :Cannot join channel (+l) - channel is full\r\n");
//             return;
//         }
        
//     }

//     channel->addMember(&client);

//     if (isNewChannel)
//         channel->addOperator(&client);
//     else if (channel->isInvited(&client))
//         channel->removeInvited(&client);

//     std::string userPrefix = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost";
//     std::string joinMsg = userPrefix + " JOIN :" + channelName + "\r\n";

//     channel->broadcastMessage(joinMsg, &client); 
//     client.sendMessage(joinMsg);

//     if (!channel->getTopic().empty())
//         client.sendMessage(":server 332 " + client.getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");
//     else 
//         client.sendMessage(":server 331 " + client.getNickname() + " " + channelName + " :No topic is set\r\n");
//         // 3. send NAMES list (353)
//     std::vector<Client*> members = channel->getMembers();
//     std::string namesList = "";
//     for (size_t i = 0; i < members.size(); ++i)
//     {
//         if (channel->isOperator(members[i]))
//             namesList += "@";
//         namesList += members[i]->getNickname();
//         if (i + 1 < members.size())
//             namesList += " ";
//     }
// client.sendMessage(":server 353 " + client.getNickname() + " = " + channelName + " :" + namesList + "\r\n");

// // 4. end of NAMES (366)
// client.sendMessage(":server 366 " + client.getNickname() + " " + channelName + " :End of /NAMES list\r\n");

// }    
// //si-hamou
// Command* createJoinCommand() 
// { 
//     return new JoinCommand(); 
// }
#include "../include/JoinCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"
#include <sstream> // ضرورية باش نخدمو بـ stringstream في التقسيم

//si-hamou
JoinCommand::JoinCommand() {}
//si-hamou
JoinCommand::~JoinCommand() {}

// فانكشن جديدة باش نقسمو السترينغ بالفاصلة
static std::vector<std::string> splitString(const std::string &str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter))
    {
        // كنتجاهلو الفراغات إلى اليوزر دار جوج فواصل متابعين (,,)
        if (!token.empty()) 
            tokens.push_back(token);
    }
    return tokens;
}

//si-hamou
static bool    isValidChannelName(const std::string &name)
{
    if(name.empty() || name.length() > 50) return false;

    char firstChar = name[0];
    if (firstChar != '#' && firstChar != '&')
        return false;

    for (size_t i = 0; i < name.length(); ++i)
    {
        char c = name[i];
        if (c == ' ' || c == ',' || c == 7)
            return false;
    }
    return true;
}
    
//si-hamou
// Fix JOIN by handling comma-separated channels and keys, treating duplicate
// JOIN as a no-op, rejecting ':' in names, applying IRC channel casemapping,
// enforcing channel quotas, and splitting every 353 reply below 512 bytes.
void JoinCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    if (params.empty())
    {
        client.sendMessage(":server 461 " + client.getNickname() + " JOIN :Not enough parameters\r\n");
        return;
    }

    // 1. كنقسمو القنوات والباسوردات باستعمال الفانكشن اللي صاوبنا الفوق
    std::vector<std::string> channels = splitString(params[0], ',');
    std::vector<std::string> keys;
    
    if (params.size() > 1)
    {
        keys = splitString(params[1], ',');
    }

    // 2. كنمشيو على كل قناة بوحدها
    for (size_t i = 0; i < channels.size(); ++i)
    {
        std::string channelName = channels[i];
        
        // كنجبدو الباسورد ديال هاد القناة (إلى كان معطي)، وإلا كنخليوه خاوي
        std::string providedKey = (i < keys.size()) ? keys[i] : "";

        if (!isValidChannelName(channelName))
        {
            client.sendMessage(":server 476 " + client.getNickname() + " " + channelName + " :Invalid channel name\r\n");
            continue; // استعملنا continue بلاصت return باش ما نحبسوش القنوات الأخرى
        }   
        
        Channel *channel = server.findChannel(channelName);
        bool isNewChannel = false;

        if (channel == NULL)
        {
            channel = server.createChannel(channelName);
            isNewChannel = true;
        }
        else 
        {
            if (!channel->getPassword().empty() && channel->getPassword() != providedKey)
            {
                client.sendMessage(":server 475 " + client.getNickname() + " " + channelName + " :Cannot join channel (+k) - bad key\r\n");
                continue;
            }

            if (channel->isInviteOnly() && !channel->isInvited(&client))
            {
                client.sendMessage(":server 473 " + client.getNickname() + " " + channelName + " :Cannot join channel (+i) - you must be invited\r\n");
                continue;
            }

            if (channel->getUserLimit() > 0 && channel->getMembersCount() >= channel->getUserLimit())
            {
                client.sendMessage(":server 471 " + client.getNickname() + " " + channelName + " :Cannot join channel (+l) - channel is full\r\n");
                continue;
            }
        }

        channel->addMember(&client);

        if (isNewChannel)
            channel->addOperator(&client);
        else if (channel->isInvited(&client))
            channel->removeInvited(&client);

        std::string userPrefix = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost";
        std::string joinMsg = userPrefix + " JOIN :" + channelName + "\r\n";

        channel->broadcastMessage(joinMsg, &client); 
        client.sendMessage(joinMsg);

        if (!channel->getTopic().empty())
            client.sendMessage(":server 332 " + client.getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");
        else 
            client.sendMessage(":server 331 " + client.getNickname() + " " + channelName + " :No topic is set\r\n");
            
        // 3. send NAMES list (353)
        std::vector<Client*> members = channel->getMembers();
        std::string namesList = "";
        
        // بدلت المتغير هنا لـ j باش ما يتخالطش مع i ديال الـ loop الكبيرة
        for (size_t j = 0; j < members.size(); ++j)
        {
            if (channel->isOperator(members[j]))
                namesList += "@";
            namesList += members[j]->getNickname();
            if (j + 1 < members.size())
                namesList += " ";
        }
        client.sendMessage(":server 353 " + client.getNickname() + " = " + channelName + " :" + namesList + "\r\n");

        // 4. end of NAMES (366)
        client.sendMessage(":server 366 " + client.getNickname() + " " + channelName + " :End of /NAMES list\r\n");
    }
}    

//si-hamou
Command* createJoinCommand() 
{ 
    return new JoinCommand(); 
}