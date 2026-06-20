


#pragma once 


#include <iostream>

class  Server 
{
    private:
        int serversocket;
        int port;
        std::string password;
    public:
        Server(int port, std::string password);
        ~Server();

         void start();
}