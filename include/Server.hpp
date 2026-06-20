// include/Server.hpp  [PARTNER'S PART — placeholder]
#pragma once
#include <vector>
#include <map>
#include <string>
#include <poll.h>
#include "Client.hpp"
#include "Channel.hpp"
#include "Command.hpp"

class Server
{
    private:
        int                              _listenFd;
        int                              _port;
        std::string                      _password;
        std::vector<struct pollfd>       _pollFds;
        std::map<int, Client*>           _clients;
        std::map<std::string, Channel*>  _channels;
        std::map<std::string, Command*>  _commands;

        void setupListenSocket();
        void registerCommands();
        void acceptNewClient();
        void handleClientReadable(int fd);
        void handleClientWritable(int fd);
        void removeClient(int fd);
        void processLine(Client &client, const std::string &line);

    public:
        Server(int port, const std::string &password);
        ~Server();
        void run();

        Channel*     findChannel(const std::string &name);
        Channel*     createChannel(const std::string &name);
        Client*      findClientByNick(const std::string &nick);
        std::string  getPassword() const;
};