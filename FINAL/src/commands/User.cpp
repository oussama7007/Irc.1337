#include "../include/UserCommand.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"

static const std::size_t MAX_USERNAME_LENGTH = 32;

//oadouz

static bool isValidUsername(const std::string &username)
{
    if (username.empty() || username.size() > MAX_USERNAME_LENGTH)
        return false;

    for (std::size_t i = 0; i < username.size(); ++i)
    {
        if (username[i] == '@' || username[i] == '!' || username[i] == ' ' || username[i] == '\0' || username[i] == '\r' || username[i] == '\n')
            return false;
    }
    return true;
}

//oadouz
UserCommand::UserCommand()
{
}

//oadouz
UserCommand::~UserCommand()
{
}

//oadouz
void UserCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    (void)server;

    std::string displayName = "*";

    if (!client.getNickname().empty())
        displayName = client.getNickname();

    if (client.hasUserSet())
    {
        client.sendMessage(":server 462 " + displayName + " :You may not reregister\r\n");
        return;
    }

    if (params.size() < 4)
    {
        client.sendMessage(":server 461 " + displayName + " USER :Not enough parameters\r\n");
        return;
    }

    if (!isValidUsername(params[0]))
    {
        client.sendMessage(":server 468 " + displayName + " " + params[0] + " :Invalid username\r\n");
        return;
    }

    client.setUsername(params[0]);
    client.setUserSet(true);

    if (client.isRegistered())
    {
        client.sendMessage(":server 001 " + client.getNickname() + " :Welcome to the ft_irc Network, " + client.getNickname() + "!" + client.getUsername() + "@localhost\r\n");
    }
}

//oadouz
Command *createUserCommand()
{
    return new UserCommand();
}
