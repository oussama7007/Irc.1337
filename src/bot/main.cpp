#include "Bot.hpp"

#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

static const std::size_t MAX_PASSWORD_SIZE = 504;
static const std::size_t MAX_CHANNEL_NAME_SIZE = 50;

//oadouz
static bool parseBotPort(const char *rawPort, int &portOut)
{
    if (rawPort == NULL || rawPort[0] == '\0')
        return false;

    for (std::size_t i = 0; rawPort[i] != '\0'; ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(rawPort[i])))
            return false;
    }

    std::istringstream stream(rawPort);
    long parsedPort = 0;
    if (!(stream >> parsedPort))
        return false;
    if (parsedPort < 1 || parsedPort > 65535)
        return false;

    portOut = static_cast<int>(parsedPort);
    return true;
}

//oadouz
static bool containsLineControl(const std::string &value)
{
    return value.find('\r') != std::string::npos || value.find('\n') != std::string::npos;
}

//oadouz
static bool isValidBotPassword(const char *rawPassword)
{
    if (rawPassword == NULL || rawPassword[0] == '\0')
        return false;

    std::string password(rawPassword);
    if (password.size() > MAX_PASSWORD_SIZE)
        return false;
    return !containsLineControl(password);
}

//oadouz
static bool isValidChannelName(const std::string &channel)
{
    if (channel.size() < 2 || channel.size() > MAX_CHANNEL_NAME_SIZE)
        return false;
    if (channel[0] != '#' && channel[0] != '&')
        return false;

    for (std::size_t i = 0; i < channel.size(); ++i)
    {
        if (channel[i] == ' ' || channel[i] == ',' || channel[i] == ':' || channel[i] == 7 || channel[i] == '\r' || channel[i] == '\n')
            return false;
    }
    return true;
}

//oadouz
int main(int argc, char **argv)
{
    if (argv == NULL || argc != 5)
    {
        std::cerr << "Usage: ./ircbot <host> <port> <password> <channel>" << std::endl;
        std::cerr << "Example: ./ircbot 127.0.0.1 6687 Passw@rd123 '#general'" << std::endl;
        return 1;
    }

    for (int i = 0; i < argc; ++i)
    {
        if (argv[i] == NULL)
        {
            std::cerr << "Error: bot received a missing argument." << std::endl;
            return 1;
        }
    }

    int port = 0;
    if (!parseBotPort(argv[2], port))
    {
        std::cerr << "Error: bot port must be a number between 1 and 65535." << std::endl;
        return 1;
    }
    if (!isValidBotPassword(argv[3]))
    {
        std::cerr << "Error: bot password must contain 1 to 504 characters without CR or LF." << std::endl;
        return 1;
    }

    std::string channel(argv[4]);
    if (!isValidChannelName(channel))
    {
        std::cerr << "Error: bot channel must be a valid # or & channel name." << std::endl;
        return 1;
    }

    try
    {
        Bot bot(argv[1], port, argv[3], channel);
        bot.run();
    }
    catch (const std::exception &exception)
    {
        std::cerr << "Bot error: " << exception.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Bot error: unknown exception" << std::endl;
        return 1;
    }
    return 0;
}
