// src/server/Server.cpp  [PARTNER'S PART — placeholder]
#include "../include/Server.hpp"
#include "Parser.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../include/JoinCommand.hpp"

Server::Server(int port, const std::string &password)
    : _listenFd(-1), _port(port), _password(password) {}

Server::~Server()
{
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        delete it->second;
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
        delete it->second;
    for (std::map<std::string, Command*>::iterator it = _commands.begin(); it != _commands.end(); ++it)
        delete it->second;
    if (_listenFd != -1)
        close(_listenFd);
}

void Server::setupListenSocket()
{
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0)
        throw std::runtime_error("socket() failed");

    int opt = 1;
    setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    fcntl(_listenFd, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_port);

    if (bind(_listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");
    if (listen(_listenFd, SOMAXCONN) < 0)
        throw std::runtime_error("listen() failed");
 
    struct pollfd pfd;
    pfd.fd = _listenFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollFds.push_back(pfd);
}

void Server::registerCommands()
{
    _commands["PASS"] = createPassCommand();
    _commands["NICK"] = createNickCommand();
    _commands["USER"] = createUserCommand();
    _commands["JOIN"] = createJoinCommand();
    // add one line here every time you finish a new command:
    // _commands["PART"]    = createPartCommand();
    // _commands["PRIVMSG"] = createPrivmsgCommand();
    // _commands["KICK"]    = createKickCommand();
    // _commands["INVITE"]  = createInviteCommand();
    // _commands["TOPIC"]   = createTopicCommand();
    // _commands["MODE"]    = createModeCommand();
}

void Server::run()
{
    setupListenSocket();
    registerCommands();
    std::cout << "ft_irc listening on port " << _port << std::endl;

    while (true)
    {
        int ready = poll(_pollFds.data(), _pollFds.size(), -1);
        if (ready < 0)
        {
            if (errno == EINTR) continue;
            throw std::runtime_error("poll() failed");
        }
        //==================================================
        // If your vector is named _pollFds:
            // std::cout << "[POLL] ready=" << ready << "\n";
            // for (size_t i = 0; i < _pollFds.size(); ++i) {
            //     if (_pollFds[i].revents != 0) {
            //         std::cout << "[POLL] fd=" << _pollFds[i].fd
            //                 << " events=0x" << std::hex << _pollFds[i].events
            //                 << " revents=0x" << _pollFds[i].revents << std::dec << "\n";
            //     }
            // }
        //==================================================
        for (size_t i = 0; i < _pollFds.size(); ++i)
        {
            if (_pollFds[i].revents & POLLIN)
            {
                if (_pollFds[i].fd == _listenFd)
                    acceptNewClient();
                else
                    handleClientReadable(_pollFds[i].fd);
            }
            else if (_pollFds[i].revents & POLLOUT)
            {
                handleClientWritable(_pollFds[i].fd);
            }
            else if (_pollFds[i].revents & (POLLHUP | POLLERR))
            {
                if (_pollFds[i].fd != _listenFd)
                    removeClient(_pollFds[i].fd);
            }
        }
    }
}

void Server::acceptNewClient()
{
    int fd = accept(_listenFd, NULL, NULL);
    if (fd < 0)
        return;

    fcntl(fd, F_SETFL, O_NONBLOCK);

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollFds.push_back(pfd);

    _clients[fd] = new Client(fd);
    std::cout << "New client connected, fd=" << fd << std::endl;
}

void Server::handleClientReadable(int fd)
{
    char buf[4096];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    //===========================================
 

        std::cout << "[READ] recv returned n=" << n << std::endl;

        if (n > 0)
        {
            std::cout << "[DATA] ";
            std::cout.write(buf, n);
            std::cout << std::endl;
        }
    //===========================================
    if (n <= 0)
    {
        removeClient(fd);
        return;
    }

    Client &client = *_clients[fd];
    client.appendToInBuffer(std::string(buf, n));

    while (client.hasCompleteLine())
        processLine(client, client.popLine());
}

void Server::processLine(Client &client, const std::string &line)
{
    std::cout << "[LINE] " << line << std::endl;
    Message msg = Parser::parse(line);
    if (msg.command.empty())
        return;

    std::map<std::string, Command*>::iterator it = _commands.find(msg.command);
    if (it == _commands.end())
        client.sendMessage(":server 421 " + client.getNickname() + " " + msg.command + " :Unknown command\r\n");
    else
        it->second->execute(*this, client, msg.params);

    // if (client.hasDataToSend())
    //     for (size_t i = 0; i < _pollFds.size(); ++i)
    //         if (_pollFds[i].fd == client.getFd())
    //             _pollFds[i].events |= POLLOUT;
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second->hasDataToSend())
        {
            for (size_t i = 0; i < _pollFds.size(); ++i)
            {
                if (_pollFds[i].fd == it->first)
                    _pollFds[i].events |= POLLOUT;
            }
        }
    }
}

void Server::handleClientWritable(int fd)
{
    Client &client = *_clients[fd];
    std::string &out = client.getOutBuffer();

    ssize_t n = send(fd, out.c_str(), out.size(), 0);
    if (n > 0)
        client.clearOutBuffer(n);

    if (!client.hasDataToSend())
        for (size_t i = 0; i < _pollFds.size(); ++i)
            if (_pollFds[i].fd == fd)
                _pollFds[i].events &= ~POLLOUT; // stop asking for POLLOUT once nothing's queued
}

void Server::removeClient(int fd)
{
    close(fd);
    delete _clients[fd];
    _clients.erase(fd);

    for (size_t i = 0; i < _pollFds.size(); ++i)
        if (_pollFds[i].fd == fd)
        {
            _pollFds.erase(_pollFds.begin() + i);
            break;
        }
}

Channel* Server::findChannel(const std::string &name)
{
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    return it == _channels.end() ? NULL : it->second;
}

Channel* Server::createChannel(const std::string &name)
{
    Channel *c = new Channel(name);
    _channels[name] = c;
    return c;
}

Client* Server::findClientByNick(const std::string &nick)
{
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        if (it->second->getNickname() == nick)
            return it->second;
    return NULL;
}

std::string Server::getPassword() const { return _password; }