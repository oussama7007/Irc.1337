



#include "../include/JoinCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"
#include "../include/Channel.hpp"


JoinCommand::JoinCommand() {}
JoinCommand::~JoinCommand() {}

bool    isValidChannelName(const std::string &name)
{
    if(name.empty() || name.length() > 50) return false;

    char firstChar = name[0];
    if (firstChar != '#' && firstChar != '&' && firstChar != '+' && firstChar != '!')
        return false;

    for (size_t i = 0; i < name.length(); ++i)
    {
        char c = name[i];
        if (c == ' ' || c == ',' || c == 7)
            return false;
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

    std::string channelName = params[0];
    if (!isValidChannelName(channelName))
    {
        client.sendMessage(":server 476 " + client.getNickname() + " " + channelName + " :Invalid channel name\r\n");
        return;
    }   
    std::string providedKey = (params.size() > 1) ? params[1] : "";
    
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
            return;
        }


        if (channel->isInviteOnly() && !channel->isInvited(&client))
        {
            client.sendMessage(":server 473 " + client.getNickname() + " " + channelName + " :Cannot join channel (+i) - you must be invited\r\n");
            return;
        }

        // 3. التحقق من العدد الأقصى (mode +l)
        // ملاحظة: ستحتاج لإضافة دالة getMembersCount() في كلاس Channel تُرجع _members.size()
        // للتبسيط الآن، إذا لم تضفها، افترض أن العدد لم يتجاوز الحد، أو أضف الدالة لتفعيل هذا الشرط.
        
        if (channel->getUserLimit() > 0 && channel->getMembersCount() >= channel->getUserLimit())
        {
            client.sendMessage(":server 471 " + client.getNickname() + " " + channelName + " :Cannot join channel (+l) - channel is full\r\n");
            return;
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
for (size_t i = 0; i < members.size(); ++i)
{
    if (channel->isOperator(members[i]))
        namesList += "@";
    namesList += members[i]->getNickname();
    if (i + 1 < members.size())
        namesList += " ";
}
client.sendMessage(":server 353 " + client.getNickname() + " = " + channelName + " :" + namesList + "\r\n");

// 4. end of NAMES (366)
client.sendMessage(":server 366 " + client.getNickname() + " " + channelName + " :End of /NAMES list\r\n");

}    
Command* createJoinCommand() 
{ 
    return new JoinCommand(); 
}