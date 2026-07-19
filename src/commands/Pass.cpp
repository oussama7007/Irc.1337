#include "../include/Pass.hpp"
#include "../include/Server.hpp"
#include "../include/Client.hpp"

static const unsigned int MAX_FAILED_PASS_ATTEMPTS = 3;

//oadouz
PassCommand::PassCommand()
{
}

//oadouz
PassCommand::~PassCommand()
{
}

//oadouz
void PassCommand::execute(Server &server, Client &client, const std::vector<std::string> &params)
{
    std::string displayName = "*";

    if (!client.getNickname().empty())
        displayName = client.getNickname();
e.
    if (client.hasPassOk())
    {
        client.sendMessage(":server 462 " + displayName + " :You may not reregister\r\n");
        return;
    }

    if (params.empty() || params[0].empty())
    {
        client.sendMessage(":server 461 " + displayName + " PASS :Not enough parameters\r\n");
        return;
    }

    if (params[0] == server.getPassword())
    {
        client.setPassOk(true);
        return;
    }

    client.recordFailedPassAttempt();
    client.sendMessage(":server 464 " + displayName + " :Password incorrect\r\n");

    if (client.getFailedPassAttempts() >= MAX_FAILED_PASS_ATTEMPTS)
        client.markClosing("too many invalid PASS attempts");
}

//oadouz
Command *createPassCommand()
{
    return new PassCommand();
}
