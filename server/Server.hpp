


#pragma once 


#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <cstdlib>
#include <cstring>

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <poll.h>

#include <fcntl.h>

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
};