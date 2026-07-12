#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Parser.hpp"
#include "Command.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <cerrno>
#include <cstring>
#include <cctype>
#include <iostream>
#include <stdexcept>

static const int LISTEN_BACKLOG = 128;

static const std::size_t RECV_CHUNK_SIZE = 4096;

static volatile sig_atomic_t g_shutdown = 0;

//oadouz
static void handleShutdownSignal(int signalNumber)
{
    (void)signalNumber;
    g_shutdown = 1;
}

Command *createPartCommand();
Command *createPrivmsgCommand();
Command *createKickCommand();
Command *createInviteCommand();
Command *createModeCommand();

//oadouz
static std::string toUpperCase(const std::string &text)
{
    std::string result = text;

    for (std::size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
    return result;
}

//oadouz
Server::Server(int port, const std::string &password)
    : _port(port),
      _password(password),
      _listenFd(-1)
{
}

//oadouz
Server::~Server()
{

    for (std::map<std::string, Command*>::iterator it = _commands.begin(); it != _commands.end(); ++it)
        delete it->second;
    _commands.clear();

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        delete it->second;
    _clients.clear();

    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
        delete it->second;
    _channels.clear();

    if (_listenFd >= 0)
    {
        close(_listenFd);
        _listenFd = -1;
    }
}

//oadouz
void Server::setupSignals()
{
    struct sigaction ignoreAction;
    struct sigaction shutdownAction;

    std::memset(&ignoreAction, 0, sizeof(ignoreAction));
    std::memset(&shutdownAction, 0, sizeof(shutdownAction));

    ignoreAction.sa_handler = SIG_IGN;

    if (sigemptyset(&ignoreAction.sa_mask) < 0)
        throw std::runtime_error("sigemptyset() failed");
    if (sigaction(SIGPIPE, &ignoreAction, NULL) < 0)
        throw std::runtime_error("sigaction(SIGPIPE) failed: SIGPIPE handling is required");

    shutdownAction.sa_handler = handleShutdownSignal;
    if (sigemptyset(&shutdownAction.sa_mask) < 0)
        throw std::runtime_error("sigemptyset() failed");

    shutdownAction.sa_flags = 0;

    if (sigaction(SIGINT, &shutdownAction, NULL) < 0)
        throw std::runtime_error("sigaction(SIGINT) failed");
    if (sigaction(SIGTERM, &shutdownAction, NULL) < 0)
        throw std::runtime_error("sigaction(SIGTERM) failed");
    if (sigaction(SIGQUIT, &shutdownAction, NULL) < 0)
        throw std::runtime_error("sigaction(SIGQUIT) failed");
}

//oadouz
void Server::setupSocket()
{

    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0)
        throw std::runtime_error(std::string("socket() failed: ") + std::strerror(errno));

    int reuseFlag = 1;

    if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &reuseFlag, sizeof(reuseFlag)) < 0)
        throw std::runtime_error(std::string("setsockopt(SO_REUSEADDR) failed: ") + std::strerror(errno));

    if (fcntl(_listenFd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error(std::string("fcntl(O_NONBLOCK) failed: ") + std::strerror(errno));

    struct sockaddr_in serverAddress;
    std::memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;

    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    serverAddress.sin_port = htons(static_cast<unsigned short>(_port));

    if (bind(_listenFd, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) < 0)
        throw std::runtime_error(std::string("bind() failed: ") + std::strerror(errno)
                                 + " (the port may be in use, or ports below 1024 may require elevated privileges)");

    if (listen(_listenFd, LISTEN_BACKLOG) < 0)
        throw std::runtime_error(std::string("listen() failed: ") + std::strerror(errno));

    struct pollfd listenPollFd;
    listenPollFd.fd = _listenFd;
    listenPollFd.events = POLLIN;
    listenPollFd.revents = 0;
    _pollFds.push_back(listenPollFd);
}

//oadouz
void Server::setupCommands()
{
    _commands["PASS"] = createPassCommand();
    _commands["NICK"] = createNickCommand();
    _commands["USER"] = createUserCommand();
    _commands["JOIN"] = createJoinCommand();
    _commands["PART"] = createPartCommand();
    _commands["PRIVMSG"] = createPrivmsgCommand();
    _commands["KICK"] = createKickCommand();
    _commands["INVITE"] = createInviteCommand();
    _commands["TOPIC"] = createTopicCommand();
    _commands["MODE"] = createModeCommand();
}

//oadouz
void Server::run()
{
    setupSignals();
    setupSocket();
    setupCommands();

    std::cout << "[ft_irc] Server is listening on port " << _port << "." << std::endl;

    while (!g_shutdown)
    {
        refreshPollEvents();

        int readyCount = poll(&_pollFds[0], _pollFds.size(), -1);
        if (readyCount < 0)
        {

            if (errno == EINTR)
                continue;

            throw std::runtime_error(std::string("poll() failed: ") + std::strerror(errno));
        }

        for (std::size_t i = 0; i < _pollFds.size(); ++i)
        {
            short returnedEvents = _pollFds[i].revents;
            int currentFd = _pollFds[i].fd;

            if (returnedEvents == 0)
                continue;

            if (currentFd == _listenFd)
            {

                if (returnedEvents & POLLIN)
                    acceptNewClient();
                else
                    throw std::runtime_error("listening socket failed (POLLERR/POLLHUP)");
                continue;
            }

            if (returnedEvents & POLLIN)
                handleClientRead(currentFd);

            if (returnedEvents & POLLOUT)
                handleClientWrite(currentFd);

            if (!(returnedEvents & POLLIN) && (returnedEvents & (POLLERR | POLLHUP | POLLNVAL)))
            {
                Client *erroredClient = getClientByFd(currentFd);
                if (erroredClient != NULL)
                    erroredClient->markDead("socket error (POLLERR/POLLHUP)");
            }
        }

        reapDeadClients();
    }

    std::cout << "[ft_irc] Shutdown signal received. Cleaning up." << std::endl;
}

//oadouz
void Server::refreshPollEvents()
{

    for (std::size_t i = 0; i < _pollFds.size(); ++i)
    {
        _pollFds[i].revents = 0;

        if (_pollFds[i].fd == _listenFd)
        {
            _pollFds[i].events = POLLIN;
            continue;
        }

        Client *client = getClientByFd(_pollFds[i].fd);

        if (client == NULL)
        {
            std::cerr << "Internal error: poll fd=" << _pollFds[i].fd
                      << " has no matching client." << std::endl;
            _pollFds[i].events = POLLIN;
            continue;
        }

        if (client->isClosing())
        {
            _pollFds[i].events = client->hasPendingOutput() ? POLLOUT : 0;
            continue;
        }

        if (client->hasPendingOutput())
            _pollFds[i].events = POLLIN | POLLOUT;
        else
            _pollFds[i].events = POLLIN;
    }
}

//oadouz
void Server::acceptNewClient()
{
    struct sockaddr_in clientAddress;
    socklen_t addressLength = sizeof(clientAddress);

    int clientFd = accept(_listenFd, reinterpret_cast<struct sockaddr*>(&clientAddress), &addressLength);

    if (clientFd < 0)
    {
        std::cerr << "Warning: accept() failed. Continuing to listen for connections." << std::endl;
        return;
    }

    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0)
    {

        std::cerr << "Error: failed to set new client socket to non-blocking mode. Closing connection."
                  << std::endl;
        close(clientFd);
        return;
    }

    char ipBuffer[INET_ADDRSTRLEN];
    const char *ipText = inet_ntop(AF_INET, &clientAddress.sin_addr, ipBuffer, sizeof(ipBuffer));

    std::string clientIp = (ipText != NULL) ? std::string(ipText) : std::string("unknown");

    Client *newClient = NULL;
    try
    {
        newClient = new Client(clientFd, clientIp);
        _clients[clientFd] = newClient;

        struct pollfd clientPollFd;
        clientPollFd.fd = clientFd;
        clientPollFd.events = POLLIN;
        clientPollFd.revents = 0;
        _pollFds.push_back(clientPollFd);
    }
    catch (const std::exception &e)
    {

        std::cerr << "Error: failed to register new client (" << e.what()
                  << "). Rolling back connection." << std::endl;
        if (newClient != NULL)
        {
            _clients.erase(clientFd);
            delete newClient;
        }
        else
            close(clientFd);
        return;
    }

    std::cout << "[+] Client connected: fd=" << clientFd << ", ip=" << clientIp << "." << std::endl;
}

//oadouz
void Server::handleClientRead(int fd)
{
    Client *client = getClientByFd(fd);

    if (client == NULL)
    {
        std::cerr << "Internal error: POLLIN event for unknown fd=" << fd << "." << std::endl;
        return;
    }

    if (client->isDead() || client->isClosing())
        return;

    char chunkBuffer[RECV_CHUNK_SIZE];
    ssize_t bytesRead = recv(fd, chunkBuffer, sizeof(chunkBuffer), 0);

    if (bytesRead == 0)
    {
        client->markClosing("client closed the connection");
        return;
    }

    if (bytesRead < 0)
    {
        client->markDead("recv() failed");
        return;
    }

    if (!client->appendToRecvBuffer(chunkBuffer, static_cast<std::size_t>(bytesRead)))
    {
        std::cerr << "Error: client fd=" << fd
                  << " exceeded the receive buffer limit without a newline. Disconnecting client."
                  << std::endl;
        client->markDead("input flood: recv buffer overflow");
        return;
    }

    std::string completedLine;

    while (client->extractLine(completedLine))
    {
        processLine(client, completedLine);

        if (client->isDead())
            break;
    }
}

//oadouz
void Server::handleClientWrite(int fd)
{
    Client *client = getClientByFd(fd);

    if (client == NULL)
    {
        std::cerr << "Internal error: POLLOUT event for unknown fd=" << fd << "." << std::endl;
        return;
    }
    if (client->isDead())
        return;

    const std::string &outgoingData = client->getSendBuffer();

    if (outgoingData.empty())
        return;

    ssize_t bytesSent = send(fd, outgoingData.c_str(), outgoingData.size(), 0);

    if (bytesSent <= 0)
    {
        client->markDead("send() failed");
        return;
    }

    client->consumeSendBuffer(static_cast<std::size_t>(bytesSent));
}

//oadouz
void Server::processLine(Client *client, const std::string &rawLine)
{

    if (client == NULL)
        return;

    Message message = Parser::parse(rawLine);

    if (message.command.empty())
        return;

    std::string commandName = toUpperCase(message.command);

    if (commandName == "CAP")
    {

        if (!message.params.empty() && toUpperCase(message.params[0]) == "LS")
            client->sendMessage(":server CAP * LS :\r\n");
        return;
    }

    if (commandName == "PING")
    {

        std::string pingToken = message.params.empty() ? std::string("ft_irc") : message.params[0];
        client->sendMessage(":server PONG server :" + pingToken + "\r\n");
        return;
    }

    if (commandName == "QUIT")
    {
        std::string quitReason = message.params.empty() ? std::string("Client Quit") : message.params[0];

        client->markClosing("Quit: " + quitReason);
        return;
    }

    std::map<std::string, Command*>::iterator commandIt = _commands.find(commandName);

    if (commandIt == _commands.end())
    {

        std::string displayName = client->getNickname().empty() ? std::string("*") : client->getNickname();
        client->sendMessage(":server 421 " + displayName + " " + commandName + " :Unknown command\r\n");
        return;
    }

    if (commandName != "PASS" && !client->hasPassOk())
    {
        client->sendMessage(":server 451 * :You have not registered\r\n");
        return;
    }

    if (commandName != "PASS" && commandName != "NICK" && commandName != "USER" && !client->isRegistered())
    {
        std::string displayName = client->getNickname().empty() ? std::string("*") : client->getNickname();
        client->sendMessage(":server 451 " + displayName + " :You have not registered\r\n");
        return;
    }

    try
    {
        commandIt->second->execute(*this, *client, message.params);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: command " << commandName << " threw an exception: " << e.what()
                  << ". Server will continue running." << std::endl;
    }
}

//oadouz
void Server::reapDeadClients()
{
    std::vector<int> deadFds;

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {

        if (it->second->isClosing() && !it->second->hasPendingOutput())
            it->second->markDead(it->second->getDeadReason());

        if (it->second->isDead())
            deadFds.push_back(it->first);
    }

    for (std::size_t i = 0; i < deadFds.size(); ++i)
        disconnectClient(deadFds[i]);
}

//oadouz
void Server::disconnectClient(int fd)
{
    std::map<int, Client*>::iterator clientIt = _clients.find(fd);

    if (clientIt == _clients.end())
        return;

    Client *client = clientIt->second;

    std::string displayName = client->getNickname().empty() ? std::string("*") : client->getNickname();
    std::cout << "[-] Client disconnected: fd=" << fd << ", nickname=" << displayName
              << ", reason=" << client->getDeadReason() << "." << std::endl;

    if (client->isRegistered())
    {
        std::string quitMessage = ":" + client->getNickname() + "!" + client->getUsername()
                                  + "@localhost QUIT :" + client->getDeadReason() + "\r\n";
        for (std::map<std::string, Channel*>::iterator chIt = _channels.begin(); chIt != _channels.end(); ++chIt)
        {

            if (chIt->second->isMember(client))
                chIt->second->broadcastMessage(quitMessage, client);
        }
    }

    for (std::map<std::string, Channel*>::iterator chIt = _channels.begin(); chIt != _channels.end(); ++chIt)
    {
        chIt->second->removeMember(client);
        chIt->second->removeInvited(client);
    }

    for (std::size_t i = 0; i < _pollFds.size(); ++i)
    {
        if (_pollFds[i].fd == fd)
        {
            _pollFds.erase(_pollFds.begin() + i);
            break;
        }
    }

    _clients.erase(clientIt);
    delete client;
}

//oadouz
Client *Server::getClientByFd(int fd)
{
    std::map<int, Client*>::iterator it = _clients.find(fd);

    if (it == _clients.end())
        return NULL;
    return it->second;
}

//oadouz
const std::string &Server::getPassword() const
{
    return _password;
}

//oadouz
Client *Server::findClientByNick(const std::string &nickname)
{

    if (nickname.empty())
        return NULL;

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {

        if (!it->second->isDead() && it->second->getNickname() == nickname)
            return it->second;
    }
    return NULL;
}

//oadouz
Channel *Server::findChannel(const std::string &name)
{
    std::map<std::string, Channel*>::iterator it = _channels.find(name);

    if (it == _channels.end())
        return NULL;
    return it->second;
}

//oadouz
Channel *Server::createChannel(const std::string &name)
{
    Channel *existingChannel = findChannel(name);

    if (existingChannel != NULL)
        return existingChannel;

    Channel *newChannel = new Channel(name);
    _channels[name] = newChannel;
    return newChannel;
}
