#pragma once

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <cstddef>

class Client;
class Channel;
class Command;

class Server
{
    private:
        int                             _port;
        std::string                     _password;
        int                             _listenFd;

        std::vector<struct pollfd>      _pollFds;
        std::map<int, Client*>          _clients;
        std::map<std::string, Channel*> _channels;
        std::map<std::string, Command*> _commands;

        Server(const Server &other);
        Server &operator=(const Server &other);

        void        setupSignals();
        void        setupSocket();
        void        setupCommands();

        void        refreshPollEvents();
        void        acceptNewClient();
        void        handleClientRead(int fd);
        void        handleClientWrite(int fd);
        void        processLine(Client *client, const std::string &rawLine);
        void        expireUnregisteredClients();
        void        reapDeadClients();
        void        disconnectClient(int fd);

        Client      *getClientByFd(int fd);
        std::size_t countClientsFromIp(const std::string &ip) const;

    public:
        Server(int port, const std::string &password);
        ~Server();

        void        run();
        void        removeChannel(const std::string& name);
        const std::string   &getPassword() const;
        Client              *findClientByNick(const std::string &nickname);
        void                broadcastNicknameChange(Client *client,
                                                    const std::string &oldNickname,
                                                    const std::string &newNickname);
        Channel             *findChannel(const std::string &name);
        Channel             *createChannel(const std::string &name);
};
