


#include "Server.hpp"

Server::Server(int port, std::string password) : serversocket(-1), port(port), password(password) {}

Server::~Server()
{
    if(serversocket != -1)
        close(serversocket);
}


void Server::start()
{
        serversocket = socket(
            AF_INET, SOCK_STREAM, 0 
        );
        if (serversocket < 0)
        {
            std::cerr << "Socket creation failed" << std::endl;
            return;
        }
        std::cout << "Socket created: "
          << serversocket
          << std::endl;
}
