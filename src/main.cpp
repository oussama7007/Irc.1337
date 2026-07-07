#include "../include/Server.hpp"
#include <iostream>
#include <cstdlib> 

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    int port = std::atoi(*(argv + 1));
    
    if (port < 1024 || port > 65535)
    {
        std::cerr << "Error: Port must be a number between 1024 and 65535." << std::endl;
        return 1;
    }

  
    std::string password = *(argv + 2);

    try
    {
        Server server(port, password);
        
        std::cout << "Starting IRC server on port " << port << "..." << std::endl;
        
        server.run(); 
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
