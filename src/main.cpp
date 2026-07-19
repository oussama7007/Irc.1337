#include "Server.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cctype>


static const std::size_t MAX_SERVER_PASSWORD_SIZE = 504;

//oadouz
static bool parsePort(const char *rawArgument, int &portOut)
{
	if (rawArgument == NULL || rawArgument[0] == '\0')
		return false;


	for (std::size_t i = 0; rawArgument[i] != '\0'; ++i)
	{
		if (!std::isdigit(static_cast<unsigned char>(rawArgument[i])))
			return false;
	}

	std::istringstream portStream(rawArgument);
	long parsedValue = 0;

	if (!(portStream >> parsedValue))
		return false;

	if (parsedValue < 1 || parsedValue > 65535)
		return false;

	portOut = static_cast<int>(parsedValue);
	return true;
}

//oadouz
static bool isValidPassword(const char *rawPassword)
{
	if (rawPassword == NULL || rawPassword[0] == '\0')
		return false;

	std::string password(rawPassword);

	if (password.size() > MAX_SERVER_PASSWORD_SIZE)
		return false;

	if (password.find('\r') != std::string::npos || password.find('\n') != std::string::npos)
		return false;

	return true;
}

//oadouz
int main(int argc, char **argv)
{
	// atexit(f);
	if (argv == NULL || argc != 3 || argv[0] == NULL)
	{
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		std::cerr << "Example: ./ircserv 6687 Passw@ord" << std::endl;
		return 1;
	}

	if (argv[1] == NULL || argv[2] == NULL)
	{
		std::cerr << "Error: port and password arguments are required." << std::endl;
		return 1;
	}

	int port = 0;
	if (!parsePort(argv[1], port))
	{
		std::cerr << "Error: invalid port '" << argv[1] << "'. Expected a number between 1 and 65535." << std::endl;
		return 1;
	}

	if (!isValidPassword(argv[2]))
	{
		std::cerr << "Error: password must contain 1 to " << MAX_SERVER_PASSWORD_SIZE << " characters and must not contain CR or LF." << std::endl;
		return 1;
	}

	std::string password = argv[2];

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
