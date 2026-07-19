#include "../include/NickCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"


static const std::size_t MAX_NICKNAME_LENGTH = 9;

//oadouz
static bool isValidNickname(const std::string &nickname)
{
    if (nickname.empty() || nickname.length() > MAX_NICKNAME_LENGTH)
        return false;

    const std::string validFirstCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[]\\`_^{|}";
    const std::string validRemainingCharacters = validFirstCharacters + "0123456789-";

    if (validFirstCharacters.find(nickname[0]) == std::string::npos)
        return false;

    for (std::size_t index = 1; index < nickname.length(); ++index)
    {
        if (validRemainingCharacters.find(nickname[index]) == std::string::npos)
            return false;
    }

    return true;
}

//oadouz
NickCommand::NickCommand()
{
}

//oadouz
NickCommand::~NickCommand()
{
}

//oadouz

void NickCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    if (params.empty() || params[0].empty())
    {
        client.sendMessage(":server 431 * :No nickname given\r\n");
        return;
    }

    const std::string newNickname = params[0];
    std::string currentNickname = "*";

    if (!client.getNickname().empty())
        currentNickname = client.getNickname();

    if (!isValidNickname(newNickname))
    {
        client.sendMessage(":server 432 " + currentNickname + " " + newNickname + " :Erroneous nickname\r\n");
        return;
    }

    Client *nicknameOwner = server.findClientByNick(newNickname);
    if (nicknameOwner != NULL && nicknameOwner != &client)
    {
        client.sendMessage(":server 433 " + currentNickname + " " + newNickname + " :Nickname is already in use\r\n");
        return;
    }

    const bool wasRegistered = client.isRegistered();
    const std::string oldNickname = client.getNickname();

    client.setNickname(newNickname);
    client.setNickSet(true);

    if (wasRegistered && oldNickname != newNickname)
        server.broadcastNicknameChange(&client, oldNickname, newNickname);


    if (!wasRegistered && client.isRegistered())
    {
        client.sendMessage(":server 001 " + client.getNickname() + " :Welcome to the ft_irc Network, " + client.getNickname() + "!" + client.getUsername() + "@localhost\r\n");
    }
}

//oadouz
Command *createNickCommand()
{
    return new NickCommand();
}
