

#include "../include/PartCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"
#include <sstream> // ضرورية للتقسيم

//si-hamou
PartCommand::PartCommand() {}
//si-hamou
PartCommand::~PartCommand() {}

// فانكشن التقسيم (نفسها اللي درنا في JOIN و KICK)
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

void PartCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    // التأكد من المعطيات[cite: 4]
    if (params.empty())
    {
        client.sendMessage(":server 461 " + client.getNickname() + " PART :Not enough parameters\r\n");
        return;
    }

    // تقسيم القنوات بالفاصلة
    std::vector<std::string> channels = splitString(params[0], ',');
    
    // تحديد رسالة الخروج[cite: 4]
    std::string partMessage = "Leaving"; 
    if (params.size() > 1)
    {
        partMessage = params[1];
    }

    // كنمشيو على كل قناة طلب اليوزر يخرج منها
    for (size_t i = 0; i < channels.size(); ++i)
    {
        std::string channelName = channels[i];
        Channel *channel = server.findChannel(channelName);
        
        // التحقق من وجود القناة[cite: 4]
        if (channel == NULL)
        {
            client.sendMessage(":server 403 " + client.getNickname() + " " + channelName + " :No such channel\r\n");
            continue; // كندوزو للقناة اللي موراها
        }

        // التحقق من أن اليوزر عضو فيها[cite: 4]
        if (!channel->isMember(&client))
        {
            client.sendMessage(":server 442 " + client.getNickname() + " " + channelName + " :You're not on that channel\r\n");
            continue;
        }

        // إرسال الميساج[cite: 4]
        std::string msgToSend = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost PART " + channelName + " :" + partMessage + "\r\n";

        channel->broadcastMessage(msgToSend, &client);
        client.sendMessage(msgToSend);

        // حذف اليوزر من القناة[cite: 4]
        channel->removeMember(&client);

        // === حماية القنوات الخاوية ===
        // نفترضو بلي عندك فانكشن getMembersCount() في كلاس Channel
        if (channel->getMembersCount() == 0) 
        {
            // هاد الفانكشن خاص الصديق ديالك يكون مقادها في Server.cpp
            // كتمسح القناة من الـ map وكدير ليها delete
            server.removeChannel(channelName); 
        }
    }
}

Command* createPartCommand() 
{ 
    return new PartCommand(); 
}