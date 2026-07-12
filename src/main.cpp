#include "Server.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

//oadouz
static bool parsePort(const char *rawArgument, int &portOut)
{

    if (rawArgument == NULL)
        return false;

    std::istringstream portStream(rawArgument);
    long parsedValue = 0;

    if (!(portStream >> parsedValue))
        return false;

    char leftoverCharacter;

    if (portStream >> leftoverCharacter)
        return false;

    if (parsedValue < 1 || parsedValue > 65535)
        return false;

    portOut = static_cast<int>(parsedValue);
    return true;
}

//oadouz
int main(int argc, char **argv)
{

    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        std::cerr << "Example: ./ircserv 6687 7mida_7ayawan" << std::endl;
        return 1;
    }

    int port = 0;
    if (!parsePort(argv[1], port))
    {

        std::cerr << "Error: invalid port '" << argv[1]
                  << "'. Expected a number between 1 and 65535." << std::endl;
        return 1;
    }

    std::string password = argv[2];

    if (password.empty())
    {
        std::cerr << "Error: password must not be empty." << std::endl;
        return 1;
    }

    try
    {
        Server server(port, password);
        server.run();
    }
    catch (const std::exception &exception)
    {

        std::cerr << "Fatal error: " << exception.what() << std::endl;
        return 1;
    }
    catch (...)
    {

        std::cerr << "Fatal error: unknown exception" << std::endl;
        return 1;
    }

    return 0;
}
