#include "../include/PrivmsgCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"
#include <sstream>

PrivmsgCommand::PrivmsgCommand() {}

PrivmsgCommand::~PrivmsgCommand() {}

// ---------------------------------------------------------
// الفانكشن 1: كتقسم لينا الكلمات اللي بيناتهم فاصلة
// ---------------------------------------------------------
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

// ---------------------------------------------------------
// الفانكشن 2: كتأكد واش هاد الهدف عبارة عن قناة (Channel)
// ---------------------------------------------------------
static bool isChannel(const std::string &target)
{
    if (target.empty()) 
    {
        return false;
    }

    char firstChar = target[0];
    
    // في الـ IRC، القنوات كيبداو بهاد الرموز
    if (firstChar == '#' || firstChar == '&' || firstChar == '+' || firstChar == '!') 
    {
        return true;
    }
    
    return false;
}

// ---------------------------------------------------------
// تنفيذ الكوماند PRIVMSG
// ---------------------------------------------------------
void PrivmsgCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    // الخطوة 1: التأكد واش اليوزر كتب لمن غيصيفط
    if (params.empty())
    {
        client.sendMessage(":server 411 " + client.getNickname() + " :No recipient given (PRIVMSG)\r\n");
        return;
    }
    
    // الخطوة 2: التأكد واش اليوزر كتب الميساج
    if (params.size() < 2)
    {
        client.sendMessage(":server 412 " + client.getNickname() + " :No text to send\r\n");
        return;
    }
 
    // الخطوة 3: أخذ المعطيات وتقسيم الأهداف
    std::string targetString = params[0];
    std::string messageText = params[1];
    
    std::vector<std::string> targetList = splitString(targetString, ',');

    // تجهيز مقدمة الرسالة (شكون اللي صيفطها)
    std::string senderPrefix = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost";

    // الخطوة 4: إرسال الميساج لكل هدف بوحدو
    for (size_t i = 0; i < targetList.size(); ++i)
    {
        std::string currentTarget = targetList[i];
        
        // تجهيز الميساج النهائي اللي غيوصل
        std::string fullMessage = senderPrefix + " PRIVMSG " + currentTarget + " :" + messageText + "\r\n";

        // ==========================================
        // الحالة الأولى: إلى كان الهدف عبارة عن قناة
        // ==========================================
        if (isChannel(currentTarget))
        {
            Channel *channel = server.findChannel(currentTarget);
            
            // واش القناة كاينة؟
            if (channel == NULL)
            {
                client.sendMessage(":server 401 " + client.getNickname() + " " + currentTarget + " :No such nick/channel\r\n");
                continue; // كندوزو للهدف اللي موراه بلا ما نحبسو
            }

            // واش اليوزر عضو في هاد القناة باش يقدر يصيفط ليها؟
            if (!channel->isMember(&client))
            {
                client.sendMessage(":server 404 " + client.getNickname() + " " + currentTarget + " :Cannot send to channel\r\n");
                continue; 
            }

            // إرسال الرسالة للجميع
            channel->broadcastMessage(fullMessage, &client);
        }
        
        // ==========================================
        // الحالة الثانية: إلى كان الهدف عبارة عن يوزر (شخص)
        // ==========================================
        else 
        {
            Client *targetClient = server.findClientByNick(currentTarget);
            
            // واش هاد الشخص مكونيكطي في السيرفر؟
            if (targetClient == NULL)
            {
                client.sendMessage(":server 401 " + client.getNickname() + " " + currentTarget + " :No such nick/channel\r\n");
                continue; 
            }
            
            // إرسال الرسالة للشخص
            targetClient->sendMessage(fullMessage);
        }
    }
}

Command* createPrivmsgCommand() 
{ 
    return new PrivmsgCommand(); 
}